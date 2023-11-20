#ifndef MZD_UTIL_H
#define MZD_UTIL_H

#define MZD_FIRST   0x1
#define MZD_VERIFY  0x2
#define MZD_CLOSE   0x4
#define MZD_KEYBIND 0x8

#define MZD_DJB2_IN_CURRENT_WORKSPACE 5461102616856308236ul  // "in_current_workspace"
#define MZD_DJB2_WM_CLASS             7573100978736958ul     // "wm_class"
#define MZD_DJB2_WM_CLASS_INSTANCE    10238665757496925234ul // "wm_class_instance"
#define MZD_DJB2_PID                  193502530ul            // "pid"
#define MZD_DJB2_ID                   5863474ul              // "id"
#define MZD_DJB2_FRAME_TYPE           8246325100528097585ul  // "frame_type"
#define MZD_DJB2_WINDOW_TYPE          13899948338458281438ul // "window_type"
#define MZD_DJB2_FOCUS                210712614469ul         // "focus"

#define mzd_flags_set(flags, flag)   (flags |=  flag)
#define mzd_flags_unset(flags, flag) (flags &= ~flag)
#define mzd_flags_has(flags, flag)   ((flags & flag) != 0)

#define mzd_nanoseconds_ms(x) x * 1000000L
#define mzd_nanoseconds_s(x) mzd_nanoseconds_ms(x * 1000)

const char **mzd_str_split(const char *str, const char delim, void **ptr);
unsigned long mzd_str_djb2(const char *str);

int mzd_strptr_cmp(const void *a, const void *b);

void mzd_ptrptr_free(void **ptrptr);
void mzd_strv_free(char **strv);

#endif
