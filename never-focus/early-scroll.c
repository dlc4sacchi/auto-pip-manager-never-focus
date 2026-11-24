#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Resolve the compositor's already loaded libraries. The preload must not
 * pull in a second copy of Clutter after a GNOME Shell update. */
typedef size_t GType;
typedef unsigned int guint;
typedef int gboolean;
typedef struct _ClutterActor ClutterActor;
typedef struct _ClutterStage ClutterStage;
typedef union _ClutterEvent ClutterEvent;
typedef struct _GSignalInvocationHint GSignalInvocationHint;
typedef struct _GValue GValue;
typedef struct _GClosure GClosure;
typedef struct _GEnumClass GEnumClass;
typedef struct {
    int value;
    const char *value_name;
    const char *value_nick;
} GEnumValue;
typedef gboolean (*EventFilter)(const ClutterEvent *, ClutterActor *, void *);
typedef void (*DestroyNotify)(void *);
typedef guint (*AddFilter)(ClutterStage *, EventFilter, DestroyNotify, void *);
typedef gboolean (*SignalAccumulator)(GSignalInvocationHint *, GValue *, const GValue *, void *);
typedef void (*SignalMarshaller)(GClosure *, GValue *, guint, const GValue *, void *, void *);

enum { SIGNAL_RUN_LAST = 2, TYPE_BOOLEAN = 5 << 2 };

static guint scroll_signal;
static int scroll_type;
static int (*event_type)(const ClutterEvent *);
static ClutterStage *(*actor_get_stage)(ClutterActor *);
static gboolean (*has_handler)(void *, guint, guint, gboolean);
static void (*signal_emit)(void *, guint, guint, ...);

/* Keep the service's dedicated preload out of apps launched by the shell. */
__attribute__((constructor))
static void clear_preload_from_child_environment(void)
{
    const char *preload = getenv("LD_PRELOAD");
    Dl_info info;
    if (preload && dladdr((void *)&clear_preload_from_child_environment, &info) &&
        strcmp(preload, info.dli_fname) == 0)
        unsetenv("LD_PRELOAD");
}

static gboolean handle_early_scroll(const ClutterEvent *event, ClutterActor *actor, void *data)
{
    (void)data;
    if (!actor || event_type(event) != scroll_type)
        return 0;

    ClutterStage *stage = actor_get_stage(actor);
    if (!stage || !has_handler(stage, scroll_signal, 0, 0))
        return 0;

    gboolean handled = 0;
    signal_emit(stage, scroll_signal, 0, event, actor, &handled);
    return handled;
}

static void register_scroll_signal(void)
{
    GType (*actor_type)(void) = dlsym(RTLD_NEXT, "clutter_actor_get_type");
    GType (*stage_type)(void) = dlsym(RTLD_NEXT, "clutter_stage_get_type");
    GType (*boxed_event_type)(void) = dlsym(RTLD_NEXT, "clutter_event_get_type");
    GType (*event_enum_type)(void) = dlsym(RTLD_NEXT, "clutter_event_type_get_type");
    void *(*class_ref)(GType) = dlsym(RTLD_NEXT, "g_type_class_ref");
    void (*class_unref)(void *) = dlsym(RTLD_NEXT, "g_type_class_unref");
    GEnumValue *(*enum_value)(GEnumClass *, const char *) = dlsym(RTLD_NEXT, "g_enum_get_value_by_name");
    SignalAccumulator true_handled = dlsym(RTLD_NEXT, "g_signal_accumulator_true_handled");
    guint (*signal_new)(const char *, GType, int, guint, SignalAccumulator, void *,
                        SignalMarshaller, GType, guint, ...) = dlsym(RTLD_NEXT, "g_signal_new");

    event_type = dlsym(RTLD_NEXT, "clutter_event_type");
    actor_get_stage = dlsym(RTLD_NEXT, "clutter_actor_get_stage");
    has_handler = dlsym(RTLD_NEXT, "g_signal_has_handler_pending");
    signal_emit = dlsym(RTLD_NEXT, "g_signal_emit");
    if (!actor_type || !stage_type || !boxed_event_type || !event_enum_type ||
        !class_ref || !class_unref || !enum_value || !true_handled || !signal_new ||
        !event_type || !actor_get_stage || !has_handler || !signal_emit)
        return;

    GEnumClass *event_class = class_ref(event_enum_type());
    if (!event_class)
        return;
    const GEnumValue *scroll = enum_value(event_class, "CLUTTER_SCROLL");
    if (scroll) {
        scroll_type = scroll->value;
        scroll_signal = signal_new("pip-before-client-scroll", stage_type(), SIGNAL_RUN_LAST,
                                   0, true_handled, NULL, NULL, TYPE_BOOLEAN, 2,
                                   boxed_event_type(), actor_type());
    }
    class_unref(event_class);
}

guint clutter_event_add_filter(ClutterStage *stage, EventFilter filter,
                              DestroyNotify destroy, void *data)
{
    static AddFilter add_filter;
    if (!add_filter) {
        add_filter = dlsym(RTLD_NEXT, "clutter_event_add_filter");
        if (!add_filter)
            return 0;

        /* Clutter runs filters in registration order. Install the hook before
         * Mutter registers the filter that forwards events to Wayland clients. */
        register_scroll_signal();
        if (scroll_signal)
            add_filter(NULL, handle_early_scroll, NULL, NULL);
    }

    return add_filter(stage, filter, destroy, data);
}
