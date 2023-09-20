extern "C" {
    #define LUA_CORE
    #include "lj_def.h"
    #define LJ_STATIC_ASSERT(cond)
    #include "lj_meta.h"
    #include "lj_str.h"
    #include "lj_tab.h"
    #include "lj_state.h"
    #include "lj_gc.h"
    #include "lj_strscan.h"
    #include "lj_strfmt.h"
    #include "lj_udata.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

namespace gpg
{
    class RRef {public: void* d; void* t;};
};

class LuaState;

class LUA_API LuaStackObject {
  public:
    LuaState* m_state;
    int m_stackIndex;

    LuaStackObject(LuaState* state, int stackIndex);
};

class LUA_API LuaObject {
  public:
    LuaObject* m_next;
    LuaObject* m_prev;
    LuaState* m_state;
    TValue m_object;

    void AddToUsedList(LuaState* state);
    void RemoveFromUsedList();
    void UpdateUsedList(LuaState* state);

    LuaObject();
    LuaObject(LuaState* state);
    LuaObject(LuaState* state, const TValue* value);
    LuaObject(const LuaStackObject& src);
    LuaObject(LuaState* state, int stackIndex);
    LuaObject(const LuaObject& src);
    LuaObject& operator=(const LuaObject& src);
    LuaObject& operator=(const LuaStackObject& src);
    ~LuaObject();
    void Reset();

    lua_State* GetActiveCState() const;
    LuaState* GetActiveState() const;

    void TypeError(const char* msg) const;

    int GetN() const;
    void SetN(int n) const;

    void Register(const char* name, lua_CFunction func, int nupvalues);

    void SetNil(const char* key) const;
    void SetBoolean(const char* key, bool value) const;
    void SetInteger(const char* key, int value) const;
    void SetInteger(int key, int value) const;
    void SetNumber(const char* key, float value) const;
    void SetNumber(int key, float value) const;
    void SetString(const char* key, const char* value) const;
    void SetString(int key, const char* value) const;
    void SetObject(const char* key,const LuaObject& value) const;
    void SetObject(int key, const LuaObject& value) const;
    void SetObject(const LuaObject& key, const LuaObject& value) const;
    void SetMetaTable(const LuaObject& value);
    LuaObject CreateTable(const char* key, int narray, int lnhash) const;
    LuaObject CreateTable(int key, int narray, int lnhash) const;

    void Insert(const LuaObject& obj) const;
    void Insert(int index, const LuaObject& obj) const;

    int GetCount() const;

    LuaObject GetByObject(const LuaObject& key) const;

    LuaObject operator[](const char* key) const;
    LuaObject operator[](int index) const;

    int Type() const;
    const char* TypeName() const;

    bool IsConvertibleToString() const;

    bool IsNil() const;
    bool IsBoolean() const;
    bool IsInteger() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsTable() const;
    bool IsFunction() const;
    bool IsUserData() const;
    int IsPassed() const;

    float ToNumber();
    const char* ToString();

    bool GetBoolean() const;
    int GetInteger() const;
    float GetNumber() const;
    const char* GetString() const;
    LuaObject GetMetaTable() const;

    void PushStack(lua_State* L) const;
    LuaStackObject PushStack(LuaState* state) const;

    void AssignNil(LuaState* state);
    void AssignBoolean(LuaState* state, bool value);
    void AssignInteger(LuaState* state, int value);
    void AssignNumber(LuaState* state, float value);
    void AssignString(LuaState* state, const char* value);
    void AssignNewTable(LuaState* state, int narray, int numhash);
    void AssignThread(LuaState* state);
    void AssignTObject(LuaState* state, const TValue* value);

    gpg::RRef LuaObject::AssignNewUserData(LuaState* state, const gpg::RRef &RRef);
    gpg::RRef LuaObject::AssignNewUserData(LuaState* state, const void* RType);
    gpg::RRef LuaObject::GetUserData() const;

    LuaObject Lookup(const char* key) const;
};

class LUA_API LuaState {
  public:
    lua_State* L; //+0x0
    void* ForMultipleThreads; //+0x4
    LuaObject* m_headObject;
    LuaObject m_threadObj; //+0xC
    LuaState* m_rootState; //+0x20

    LuaState(int initLibs);
    LuaState(LuaState* parentState);
    ~LuaState();

    LuaObject GetGlobals();
    LuaObject GetGlobal(const char* key);

    lua_State* GetActiveCState();

    int ArgError(int narg, const char* msg);
    int __cdecl Error(const char* fmt, ...);
};

void LuaPlusGCFunction(void* s) {
    global_State* g = (global_State*)s;
    LuaState* m_state = (LuaState*)mainthread(g)->stateUserData;
    LuaObject* obj = m_state->m_headObject;
    while (obj) {
        if (tvisgcv(&obj->m_object))
            gc_marktv(g, &obj->m_object);
        obj = obj->m_prev;
    }
}

void ThrowException(void* s, int errcode) {
    typedef void* (__thiscall *_luaError_Init)(void* t, void* L, int i);
    const auto luaError_Init = (_luaError_Init)0x90DA40;
    typedef void (__stdcall *_CxxThrowException)(void *a1, void *a2);
    const auto CxxThrowException = (_CxxThrowException)0xA89950;

    auto L = (lua_State*)s;
    if (tvisstr(L->top - 1))
        luaL_traceback(L, L, lua_tostring(L, -1), 0);
    struct {void* vtable; char pad[0x2C];} luaError;
    luaError_Init(&luaError, L, 1);
    luaError.vtable = (void*)0xD45958; // .?AUlua_RuntimeError@@
    CxxThrowException(&luaError, (void*)0xEA5BE0);
}

LuaState::LuaState(int initLibs) {
    L = luaL_newstate();
    ForMultipleThreads = 0;
    m_rootState = this;
    m_headObject = NULL;
    if (initLibs) {
        //luaopen_jit(L);
        luaopen_base(L);
        luaopen_table(L);
        luaopen_string(L);
        luaopen_math(L);
        luaopen_bit(L);
        luaopen_debug(L);
        luaopen_string_buffer(L);
        if (initLibs > 1) {
            luaopen_io(L);
            luaopen_os(L);
        }
        lua_settop(L, 0);
    }
    lua_gc(L, LUA_GCSTOP, 0);
    L->stateUserData = this;
    G(L)->throwException = ThrowException;
    G(L)->userGCFunction = LuaPlusGCFunction;
    GetGlobals().Register((char*)0xE09B24,(lua_CFunction)0x90A8C0,0);
    GetGlobals().Register((char*)0xD44E98,(lua_CFunction)0x90A8C0,0);
}

LuaState::LuaState(LuaState* parentState) {
    m_rootState = parentState->m_rootState;
    L = lua_newthread(m_rootState->L);
    ForMultipleThreads = 0;
    m_threadObj = LuaObject(m_rootState, --m_rootState->L->top);
    m_headObject = NULL;
    L->stateUserData = this;
}

LuaState::~LuaState() {
    LuaObject* obj = m_headObject;
    while (obj) {
        obj->Reset();
        obj = obj->m_prev;
    }
    L->stateUserData = NULL;
    if (m_rootState == this) lua_close(L);
}

LuaObject LuaState::GetGlobals() {
    LuaObject obj = LuaObject(this);
    settabV(L, &obj.m_object, tabref(L->env));
    return obj;
}

LuaObject LuaState::GetGlobal(const char* key) {
    return GetGlobals()[key];
}

lua_State* LuaState::GetActiveCState() {
    return mainthread(G(L));
}

int LuaState::ArgError(int narg, const char* msg) {
    return luaL_argerror(L, narg, msg);
}

int __cdecl LuaState::Error(const char* fmt, ...) {
    __asm {
        mov eax,[esp+0x4]
        mov eax,[eax]LuaState.L
        mov [esp+0x4],eax
        jmp luaL_error
    }
}

LuaStackObject::LuaStackObject(LuaState* state, int stackIndex) {
    m_state = state;
    m_stackIndex = stackIndex;
}

void LuaObject::AddToUsedList(LuaState* state) {
    m_state = state->m_rootState;
    m_prev = m_state->m_headObject;
    m_state->m_headObject = this;
    if (m_prev) m_prev->m_next = this;
    m_next = NULL;
}

void LuaObject::RemoveFromUsedList() {
    if (!m_state) return;
    if (m_prev) m_prev->m_next = m_next;
    if (m_next)
        m_next->m_prev = m_prev; else
        m_state->m_headObject = m_prev;
}

void LuaObject::UpdateUsedList(LuaState* state) {
    if (state->m_rootState == m_state) return;
    RemoveFromUsedList();
    AddToUsedList(state);
}

LuaObject::LuaObject() {
    m_state = NULL;
    setnilV(&m_object);
    m_next = NULL;
    m_prev = NULL;
}

LuaObject::LuaObject(LuaState* state) {
    AddToUsedList(state);
    setnilV(&m_object);
}

LuaObject::LuaObject(LuaState* state, const TValue* value) {
    AddToUsedList(state);
    lua_State* L = m_state->L;
    copyTV(L, &m_object, value);
}

LuaObject::LuaObject(const LuaStackObject& src) {
    AddToUsedList(src.m_state);
    lua_State* L = src.m_state->L;
    copyTV(L, &m_object, (TValue*)index2adrF(L, src.m_stackIndex));
}

LuaObject::LuaObject(LuaState* state, int stackIndex) {
    AddToUsedList(state);
    lua_State* L = state->L;
    copyTV(L, &m_object, (TValue*)index2adrF(L, stackIndex));
}

LuaObject::LuaObject(const LuaObject& src) {
    if (src.m_state) {
        AddToUsedList(src.m_state);
        lua_State* L = src.m_state->L;
        copyTV(L, &m_object, &src.m_object);
    } else {
        m_state = NULL;
        setnilV(&m_object);
        m_next = NULL;
        m_prev = NULL;
    }
}

LuaObject& LuaObject::operator=(const LuaObject& src) {
    UpdateUsedList(src.m_state);
    lua_State* L = src.m_state->L;
    copyTV(L, &m_object, &src.m_object);
    return *this;
}

LuaObject& LuaObject::operator=(const LuaStackObject& src) {
    UpdateUsedList(src.m_state);
    lua_State* L = src.m_state->L;
    copyTV(L, &m_object, (TValue*)index2adrF(L, src.m_stackIndex));
    return *this;
}

LuaObject::~LuaObject() {
    lua_State* L = m_state->L;
    checklivetv(L, &m_object, "destroy dead GC object");
    RemoveFromUsedList();
}

void LuaObject::Reset() {
    RemoveFromUsedList();
    m_state = NULL;
    setnilV(&m_object);
}

lua_State* LuaObject::GetActiveCState() const {
    return mainthread(G(m_state->L));
}

LuaState* LuaObject::GetActiveState() const {
    return (LuaState*)GetActiveCState()->stateUserData;
}

void LuaObject::TypeError(const char* msg) const {
    luaL_typerror(m_state->L, 0, msg);
}

int LuaObject::GetN() const {
    return lj_tab_len(tabV(&m_object));
}

void LuaObject::SetN(int n) const {
    return;
}

void LuaObject::Register(const char* name, lua_CFunction func, int nupvalues) {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, name));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    lua_pushcclosure(L, func, nupvalues);
    copyTV(L, v, --L->top);
}

void LuaObject::SetNil(const char* key) const {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setnilV(v);
}

void LuaObject::SetBoolean(const char* key, bool value) const {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setboolV(v, value);
}

void LuaObject::SetInteger(const char* key, int value) const {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setintV(v, value);
}

void LuaObject::SetInteger(int key, int value) const {
    TValue k;
    lua_State* L = m_state->L;
    setintV(&k, key);
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setintV(v, value);
}

void LuaObject::SetNumber(const char* key, float value) const {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setnumV(v, value);
}

void LuaObject::SetNumber(int key, float value) const {
    TValue k;
    lua_State* L = m_state->L;
    setintV(&k, key);
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setnumV(v, value);
}

void LuaObject::SetString(const char* key, const char* value) const {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setstrV(L, v, lj_str_newz(L, value));
}

void LuaObject::SetString(int key, const char* value) const {
    TValue k;
    lua_State* L = m_state->L;
    setintV(&k, key);
    TValue* v = lj_meta_tset(L, &m_object, &k);
    setstrV(L, v, lj_str_newz(L, value));
}

void LuaObject::SetObject(const char* key,const LuaObject& value) const {
    TValue k;
    lua_State* L = m_state->L;
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    copyTV(L, v, &value.m_object);
}

void LuaObject::SetObject(int key, const LuaObject& value) const {
    TValue k;
    lua_State* L = m_state->L;
    setintV(&k, key);
    TValue* v = lj_meta_tset(L, &m_object, &k);
    copyTV(L, v, &value.m_object);
}

void LuaObject::SetObject(const LuaObject& key, const LuaObject& value) const {
    lua_State* L = m_state->L;
    TValue* v = lj_meta_tset(L, &m_object, &key.m_object);
    copyTV(L, v, &value.m_object);
}

void LuaObject::SetMetaTable(const LuaObject& value) {
    lua_State* L = m_state->L;
    copyTV(L, L->top, &m_object);
    incr_top(L);
    copyTV(L, L->top, &value.m_object);
    incr_top(L);
    lua_setmetatable(L, -2);
    --L->top;
}

LuaObject LuaObject::CreateTable(const char* key, int narray, int lnhash) const {
    TValue k;
    lua_State* L = m_state->L;
    GCtab* t = lj_tab_new(L, narray, lnhash);
    setstrV(L, &k, lj_str_newz(L, key));
    TValue* v = lj_meta_tset(L, &m_object, &k);
    settabV(L, v, t);
    return LuaObject(m_state, v);
}

LuaObject LuaObject::CreateTable(int key, int narray, int lnhash) const {
    TValue k;
    lua_State* L = m_state->L;
    GCtab* t = lj_tab_new(L, narray, lnhash);
    setintV(&k, key);
    TValue* v = lj_meta_tset(L, &m_object, &k);
    settabV(L, v, t);
    return LuaObject(m_state, v);
}

void LuaObject::Insert(const LuaObject& obj) const {
    lua_State* L = m_state->L;
    m_state->GetGlobals().Lookup("table.insert").PushStack(L);
    PushStack(L);
    obj.PushStack(L);
    lua_call(L, 2, 0);
}

void LuaObject::Insert(int index, const LuaObject& obj) const {
    lua_State* L = m_state->L;
    m_state->GetGlobals().Lookup("table.insert").PushStack(L);
    PushStack(L);
    lua_pushinteger(L, index);
    obj.PushStack(L);
    lua_call(L, 3, 0);
}

int LuaObject::GetCount() const {
    lua_State* L = m_state->L;
    PushStack(L);
    int count = lua_getn(L, lua_gettop(L));
    --L->top;
    return count;
}

LuaObject LuaObject::GetByObject(const LuaObject& key) const {
    lua_State* L = m_state->L;
    cTValue* v = lj_meta_tget(L, &m_object, &key.m_object);
    return LuaObject(m_state, v);
}

LuaObject LuaObject::operator[](const char* key) const {
    lua_State* L = m_state->L;
    GCstr* k = lj_str_newz(L, key);
    cTValue* v = lj_tab_getstr(tabV(&m_object), k);
    if (v) return LuaObject(m_state, v);
    return LuaObject(m_state);
}

LuaObject LuaObject::operator[](int index) const {
    cTValue* v = lj_tab_getint(tabV(&m_object), index);
    if (v) return LuaObject(m_state, v);
    return LuaObject(m_state);
}

int LuaObject::Type() const {
    return FAlua_type(m_state->L, (void*)&m_object);
}

const char* LuaObject::TypeName() const {
    return lua_typename(m_state->L, Type());
}

bool LuaObject::IsConvertibleToString() const {
    return tvisnum(&m_object) || tvisstr(&m_object);
}

bool LuaObject::IsNil() const {
    return tvisnil(&m_object);
}

bool LuaObject::IsBoolean() const {
    return tvisbool(&m_object);
}

bool LuaObject::IsInteger() const {
    return tvisnum(&m_object);
}

bool LuaObject::IsNumber() const {
    return tvisnum(&m_object);
}

bool LuaObject::IsString() const {
    return tvisstr(&m_object);
}

bool LuaObject::IsTable() const {
    return tvistab(&m_object);
}

bool LuaObject::IsFunction() const {
    return tvisfunc(&m_object);
}

bool LuaObject::IsUserData() const {
    return (tvisudata(&m_object) || tvislightud(&m_object));
}

int LuaObject::IsPassed() const
{
    return (m_state) && (!tvisnil(&m_object)) && (!tvisfalse(&m_object));
}

float LuaObject::ToNumber() {
    lua_State* L = m_state->L;
    PushStack(L);
    float r = lua_tonumberF(L, -1);
    --L->top;
    return r;
}

const char* LuaObject::ToString() {
    if (tvisstr(&m_object))
        return strdata(strV(&m_object));
    if (!tvisnumber(&m_object)) return NULL;
    lua_State* L = m_state->L;
    GCstr* s = lj_strfmt_number(L, &m_object);
    setstrV(L, &m_object, s);
    return strdata(s);
}

bool LuaObject::GetBoolean() const {
    return tvistruecond(&m_object);
}

int LuaObject::GetInteger() const {
    return (uint32_t)(numV(&m_object));
}

float LuaObject::GetNumber() const {
    return (float)numV(&m_object);
}

const char* LuaObject::GetString() const {
    return strdata(strV(&m_object));
}

LuaObject LuaObject::GetMetaTable() const {
    TValue* v;
    lua_State* L = m_state->L;
    PushStack(L);
    if (lua_getmetatable(L, -1))
        v = --L->top; else v = niltv(L);
    --L->top;
    return LuaObject(m_state, v);
}

void LuaObject::PushStack(lua_State* L) const {
    copyTV(L, L->top, &m_object);
    incr_top(L);
}

LuaStackObject LuaObject::PushStack(LuaState* state) const {
    lua_State* L = state->L;
    copyTV(L, L->top, &m_object);
    incr_top(L);
    return LuaStackObject(state, lua_gettop(L));
}

void LuaObject::AssignNil(LuaState* state) {
    UpdateUsedList(state);
    setnilV(&m_object);
}

void LuaObject::AssignBoolean(LuaState* state, bool value) {
    UpdateUsedList(state);
    setboolV(&m_object, value);
}

void LuaObject::AssignInteger(LuaState* state, int value) {
    UpdateUsedList(state);
    setintV(&m_object, value);
}

void LuaObject::AssignNumber(LuaState* state, float value) {
    UpdateUsedList(state);
    setnumV(&m_object, value);
}

void LuaObject::AssignString(LuaState* state, const char* value) {
    UpdateUsedList(state);
    lua_State* L = m_state->L;
    GCstr* s = lj_str_newz(L, value);
    setstrV(L, &m_object, s);
}

void LuaObject::AssignNewTable(LuaState* state, int narray, int nhash) {
    UpdateUsedList(state);
    lua_State* L = m_state->L;
    GCtab* t = lj_tab_new_ah(L, narray, nhash);
    settabV(L, &m_object, t);
}

void LuaObject::AssignThread(LuaState* state) {
    UpdateUsedList(state);
    lua_State* L = m_state->L;
    setthreadV(L, &m_object, state->L);
}

void LuaObject::AssignTObject(LuaState* state, const TValue* value) {
    UpdateUsedList(state);
    lua_State* L = m_state->L;
    copyTV(L, &m_object, value);
}

typedef void (*fRRef)(void* r, void* u, const gpg::RRef& RRef);
gpg::RRef LuaObject::AssignNewUserData(LuaState* state, const gpg::RRef &RRef)
{
    UpdateUsedList(state);
    lua_State* L = m_state->L;
    GCudata* u = lj_udata_new(L, *(size_t*)((char*)RRef.t + 8), tabref(L->env));
    gpg::RRef r;
    fRRef(*(int*)((char*)RRef.t + 0x58))(&r, uddata(u), RRef);
    setudataV(L, &m_object, u);
    u->t = r.t;
    return r;
}

typedef void (*fRType)(void* r, const void* u);
gpg::RRef LuaObject::AssignNewUserData(LuaState* state, const void* RType)
{
    UpdateUsedList(state);
    lua_State* L = m_state->L;
    GCudata* u = lj_udata_new(L, *(size_t*)((char*)RType + 8), tabref(L->env));
    gpg::RRef r;
    fRType(*(int*)((char*)RType + 0x54))(&r, uddata(u));
    setudataV(L, &m_object, u);
    u->t = r.t;
    return r;
}

gpg::RRef LuaObject::GetUserData() const
{
    GCudata* u = udataV(&m_object);
    return {uddata(u), u->t};
}

LuaObject LuaObject::Lookup(const char* key) const {
    cTValue* v = &m_object;
    char* lastPos = (char*)key;
    char* curPos;
    lua_State* L = m_state->L;
    do {
        TValue k;
        curPos = strchr(lastPos, '.');
        if (curPos)
            setstrV(L, &k, lj_str_new(L, lastPos, curPos - lastPos)); else
            setstrV(L, &k, lj_str_newz(L, lastPos));
        lj_strscan_num(strV(&k), &k);
        v = lj_meta_tget(L, v, &k);
        if (tvisnil(v)) break;
        lastPos = curPos + 1;
    } while (curPos);
    return LuaObject(m_state, v);
}

LUA_API bool LuaPlusH_next(LuaState* state, const LuaObject* table, LuaObject* key, LuaObject* value) {
    TValue objs[2];
    GCtab* t = tabV(&table->m_object);
    if (lj_tab_next(t, &key->m_object, &objs[0])) {
        key->AssignTObject(state, &objs[0]);
        value->AssignTObject(state, &objs[1]);
        return true;
    }
    return false;
}

extern "C" {
LUA_API void GetTableAH(GCtab* t, uint32_t *asize, uint8_t *hbits) {
    *asize = t->asize;
    *hbits = 0;
    uint32_t hmask = t->hmask << 1;
    while (hmask > 1) {
        hmask >>= 1;
        *hbits++;
    }
}
}