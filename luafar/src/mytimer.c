//initial source: https://qnaplus.com/implement-periodic-timer-linux/

#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "util.h"

#define MAX_TIMER_COUNT 1000

typedef enum
{
TIMER_SINGLE_SHOT = 0,
TIMER_PERIODIC
} t_timer;

typedef void (*time_handler)(size_t timer_id, void * user_data);

struct timer_node
{
    int                 fd;
    time_handler        callback;
    void *              user_data;
    unsigned int        interval;
    t_timer             type;
    struct timer_node * next;
};

static void * _timer_thread(void * data);
static pthread_t g_thread_id;
static struct timer_node *g_head = NULL;

int initialize()
{
    if(pthread_create(&g_thread_id, NULL, _timer_thread, NULL))
    {
        /*Thread creation failed*/
        return 0;
    }

    return 1;
}

size_t start_timer(unsigned int interval, time_handler handler, t_timer type, void * user_data)
{
    struct timer_node * new_node = NULL;
    struct itimerspec new_value;

    new_node = (struct timer_node *)malloc(sizeof(struct timer_node));

    if(new_node == NULL) return 0;

    new_node->callback  = handler;
    new_node->user_data = user_data;
    new_node->interval  = interval;
    new_node->type      = type;

    new_node->fd = timerfd_create(CLOCK_REALTIME, 0);

    if (new_node->fd == -1)
    {
        free(new_node);
        return 0;
    }

    new_value.it_value.tv_sec = interval / 1000;
    new_value.it_value.tv_nsec = (interval % 1000)* 1000000;

    if (type == TIMER_PERIODIC)
    {
      new_value.it_interval.tv_sec= interval / 1000;
      new_value.it_interval.tv_nsec = (interval %1000) * 1000000;
    }
    else
    {
      new_value.it_interval.tv_sec= 0;
      new_value.it_interval.tv_nsec = 0;
    }

    timerfd_settime(new_node->fd, 0, &new_value, NULL);

    /*Inserting the timer node into the list*/
    new_node->next = g_head;
    g_head = new_node;

    return (size_t)new_node;
}

void stop_timer(size_t timer_id)
{
    struct timer_node * tmp = NULL;
    struct timer_node * node = (struct timer_node *)timer_id;

    if (node == NULL) return;

    close(node->fd);

    if(node == g_head)
    {
        g_head = g_head->next;
    } else {

        tmp = g_head;

        while(tmp && tmp->next != node) tmp = tmp->next;

        if(tmp)
        {
            /*tmp->next can not be NULL here.*/
            tmp->next = tmp->next->next;
        }
    }
    if(node) free(node);
}

void finalize()
{
    while(g_head) stop_timer((size_t)g_head);

    pthread_cancel(g_thread_id);
    pthread_join(g_thread_id, NULL);
}

struct timer_node * _get_timer_from_fd(int fd)
{
    struct timer_node * tmp = g_head;

    while(tmp)
    {
        if(tmp->fd == fd) return tmp;

        tmp = tmp->next;
    }
    return NULL;
}

void * _timer_thread(void * data)
{
    struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
    int iMaxCount = 0;
    struct timer_node * tmp = NULL;
    int read_fds = 0, i, s;
    uint64_t exp;

    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        iMaxCount = 0;
        tmp = g_head;

        memset(ufds, 0, sizeof(struct pollfd)*MAX_TIMER_COUNT);
        while(tmp)
        {
            ufds[iMaxCount].fd = tmp->fd;
            ufds[iMaxCount].events = POLLIN;
            iMaxCount++;

            tmp = tmp->next;
        }
        read_fds = poll(ufds, iMaxCount, 100);

        if (read_fds <= 0) continue;

        for (i = 0; i < iMaxCount; i++)
        {
            if (ufds[i].revents & POLLIN)
            {
                s = read(ufds[i].fd, &exp, sizeof(uint64_t));

                if (s != sizeof(uint64_t)) continue;

                tmp = _get_timer_from_fd(ufds[i].fd);

                if(tmp && tmp->callback) tmp->callback((size_t)tmp, tmp->user_data);
            }
        }
    }

    return NULL;
}

typedef struct PluginStartupInfo PSInfo;

const char FarTimerType[] = "FarTimer";

void timer_handler(size_t timer_id, void *user_data)
{
  TTimerData *td = (TTimerData*) user_data;
  switch(td->closeStage) {
    case 0:
      if (td->enabled)
        td->Info->AdvControl(td->Info->ModuleNumber, ACTL_SYNCHRO, td);
      break;

    case 1:
      stop_timer(td->timer_id);
      td->closeStage++;
      td->Info->AdvControl(td->Info->ModuleNumber, ACTL_SYNCHRO, td);
      break;

    case 2:
      break;
  }
}

int far_Timer (lua_State *L)
{
  TTimerData *td;

  lua_settop(L, 2);
  td = (TTimerData*)lua_newuserdata(L, sizeof(TTimerData));
  td->interval = (unsigned)luaL_checkinteger(L, 1); //arg #1
  luaL_checktype(L, 2, LUA_TFUNCTION);              //arg #2

  lua_pushvalue(L, 2);
  td->funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_pushvalue(L, -1);
  td->objRef = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_pushthread(L);
  td->threadRef = luaL_ref(L, LUA_REGISTRYINDEX);

  td->Info = GetPluginStartupInfo(L);
  td->closeStage = 0;
  td->enabled = 0;
  td->interval_changed = 0; //TODO

  td->timer_id = start_timer(td->interval, timer_handler, TIMER_PERIODIC, td);
  if (td->timer_id) {
    luaL_getmetatable(L, FarTimerType);
    lua_setmetatable(L, -2);
    td->enabled = 1;
    return 1;
  }
  else {
    luaL_unref(L, LUA_REGISTRYINDEX, td->objRef);
    luaL_unref(L, LUA_REGISTRYINDEX, td->funcRef);
    luaL_unref(L, LUA_REGISTRYINDEX, td->threadRef);
    return lua_pushnil(L), 1;
  }
}

TTimerData* CheckTimer(lua_State* L, int pos)
{
  return (TTimerData*)luaL_checkudata(L, pos, FarTimerType);
}

TTimerData* CheckValidTimer(lua_State* L, int pos)
{
  TTimerData* td = CheckTimer(L, pos);
  luaL_argcheck(L, td->closeStage == 0, pos, "attempt to access closed timer");
  return td;
}

int timer_Close (lua_State *L)
{
  TTimerData* td = CheckTimer(L, 1);
  if (td->closeStage == 0)
    td->closeStage++;
  return 0;
}

int timer_tostring (lua_State *L)
{
  TTimerData* td = CheckTimer(L, 1);
  if (td->closeStage == 0)
    lua_pushfstring(L, "%s (%p)", FarTimerType, td);
  else
    lua_pushfstring(L, "%s (closed)", FarTimerType);
  return 1;
}

int timer_index (lua_State *L)
{
  TTimerData* td = CheckTimer(L, 1);
  const char* method = luaL_checkstring(L, 2);
  if      (!strcmp(method, "Close"))       lua_pushcfunction(L, timer_Close);
  else if (!strcmp(method, "Enabled"))     lua_pushboolean(L, td->enabled);
  else if (!strcmp(method, "Interval"))    lua_pushinteger(L, td->interval);
  else if (!strcmp(method, "OnTimer"))     lua_rawgeti(L, LUA_REGISTRYINDEX, td->funcRef);
  else if (!strcmp(method, "Closed"))      lua_pushboolean(L, td->closeStage);
  else                                     luaL_error(L, "attempt to call non-existent method");
  return 1;
}

int timer_newindex (lua_State *L)
{
  TTimerData* td = CheckValidTimer(L, 1);
  const char* method = luaL_checkstring(L, 2);
  if (!strcmp(method, "Enabled")) {
    luaL_checkany(L, 3);
    td->enabled = lua_toboolean(L, 3);
  }
  else if (!strcmp(method, "OnTimer")) {
    luaL_checktype(L, 3, LUA_TFUNCTION);
    lua_pushvalue(L, 3);
    lua_rawseti(L, LUA_REGISTRYINDEX, td->funcRef);
  }
  else luaL_error(L, "attempt to call non-existent method");
  return 0;
}

static const luaL_Reg timer_methods[] = {
  {"__gc",             timer_Close},
  {"__tostring",       timer_tostring},
  {"__index",          timer_index},
  {"__newindex",       timer_newindex},
  {NULL, NULL},
};

int finalize_timer_system(lua_State *L)
{
  (void)L;
  finalize();
  return 0;
}

int luaopen_timer(lua_State *L)
{
  if (initialize()) {
    lua_newuserdata(L, 4);             //+1 create a userdatum whose __gc will be called on destroying the lua_State
    lua_newtable(L);                   //+2 create a metatable
    lua_pushcfunction(L, finalize_timer_system); //+3
    lua_setfield(L, -2, "__gc");       //+2
    lua_setmetatable(L, -2);           //+1
    lua_pushvalue(L, -1);              //+2
    lua_rawset(L, LUA_REGISTRYINDEX);  //+0 place it in Lua registry (both as the key and the value)
    luaL_newmetatable(L, FarTimerType);
    luaL_register(L, NULL, timer_methods);
    lua_pushcfunction(L, far_Timer);
  }
  else
    lua_pushnil(L);
  return 1;
}
