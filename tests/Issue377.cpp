#undef X_MACRO
#define X_MACRO(iconid) iconid,
enum class icon_id {
    X_MACRO(action)
    X_MACRO(action2)
};

#undef X_MACRO
#define X_MACRO(iconid) case icon_id::iconid: return 0;
int to_int(icon_id id){
    switch(id){
        X_MACRO(action)
        X_MACRO(action2)
    }
}
