// server.cpp
// C++11: remote procedure call library for MQL4

#ifndef _WIN32
#define MQLAPI __attribute__((visibility("default")))
#define MQLCALL
#undef  _GLIBCXX_USE_INT128
#else
#define MQLAPI __declspec(dllexport)
#define MQLCALL __stdcall
#endif

#include "time.h"
#include <zmq.h>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <iostream>
#include <algorithm>
#include <vector>

std::vector<int> int_array;
std::vector<double> double_array;
std::vector<std::string> string_array;

struct Indicator {
    std::string name;

    std::string symbol;
    int period;

    std::vector<int> ints;
    std::vector<double> doubles;
    std::vector<std::string> strings;

    Indicator(std::string n, std::string s, int p)
        : name(n), symbol(s), period(p) {}
};

std::vector<Indicator*> indicators;

struct mql_string {
    size_t len;
    char *s;
};

extern "C" {
MQLAPI zmq::socket_t* MQLCALL r_init(int port);
MQLAPI void           MQLCALL r_finish(zmq::socket_t*);

MQLAPI int    MQLCALL r_recv_pack(zmq::socket_t*);
MQLAPI int    MQLCALL r_packet_return(zmq::socket_t*);

MQLAPI void   MQLCALL r_array_size(int*, Indicator*);
MQLAPI int    MQLCALL r_int_array(int*, Indicator*);
MQLAPI int    MQLCALL r_double_array(double*, Indicator*);
MQLAPI int    MQLCALL r_string_array(mql_string*, Indicator*);

MQLAPI void   MQLCALL r_int_array_set(int*, int);
MQLAPI void   MQLCALL r_double_array_set(double*, int);
MQLAPI void   MQLCALL r_string_array_set(mql_string*, int);

MQLAPI Indicator* MQLCALL ind_init(char*, char*, int);
MQLAPI int        MQLCALL ind_get_all(mql_string*);
MQLAPI int        MQLCALL ind_find(char *name, int *arr, mql_string *str_arr);
MQLAPI void       MQLCALL ind_finish(Indicator*);
}

zmq::socket_t* MQLCALL r_init(int port)
{
    zmq::context_t ctx(1);
    zmq::socket_t* sd = new zmq::socket_t(ctx, ZMQ_REP);
    std::cerr << "initing ";
    sd->bind("tcp://*:8000");
    std::cerr << "inited" << std::endl;
    // TODO catch return -1
    return sd;
}

void MQLCALL r_finish(zmq::socket_t* s)
{
    delete s;
}

int MQLCALL r_packet_return(zmq::socket_t c)
{
    int r;
    unsigned short len;
    void *ptr;

    try {
        std::cerr << " :: r_packet_return" << std::endl;

        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> packer(&sbuf);
        packer.pack_array(!int_array.empty() + !double_array.empty() + !string_array.empty());

        if(int_array.size() > 0)
            packer.pack(int_array);
        if(double_array.size() > 0)
            packer.pack(double_array);
        if(string_array.size() > 0)
            packer.pack(string_array);

        zmq::message_t msg(sbuf.size());
        memcpy(msg.data(), sbuf.data(), sbuf.size());

        c.send(msg);

        return 1;
    }
    catch(int wsaerr) {
        std::cerr << "send error: " << wsaerr << std::endl;
    }
    catch(std::bad_alloc&) {
        std::cerr << "send error: alloc failed" << std::endl;
    }
    catch(...) {
        std::cerr << "send: connection closed" << std::endl;
    }
    return -1;
}

void MQLCALL r_array_size(int *size, Indicator *ind)
{
    if(ind == nullptr) {
        size[0] = int_array.size();
        size[1] = double_array.size();
        size[2] = string_array.size();
    }
    else {
        size[0] = ind->ints.size();
        size[1] = ind->doubles.size();
        size[2] = ind->strings.size();
    }
}

int MQLCALL r_int_array(int* arr, Indicator *ind)
{
    auto int_arr = &int_array;
    if(ind != nullptr) {
        int_arr = &ind->ints;
    }

    std::cerr << " :: int_array " << int_arr->size() << std::endl;
    if(arr == NULL || int_arr->empty())
        return 0;
    std::copy(int_arr->begin(), int_arr->end(), arr);
    return int_arr->size();
}

int MQLCALL r_double_array(double* arr, Indicator *ind)
{
    auto dbl_arr = &double_array;
    if(ind != nullptr) {
        dbl_arr = &ind->doubles;
    }

    std::cerr << " :: double_array " << dbl_arr->size() << std::endl;
    if(arr == NULL || dbl_arr->empty())
        return 0;
    std::copy(dbl_arr->begin(), dbl_arr->end(), arr);
    return dbl_arr->size();
}

int MQLCALL r_string_array(mql_string *arr, Indicator *ind)
{
    auto str_arr = &string_array;
    if(ind != nullptr) {
        str_arr = &ind->strings;
    }

    std::cerr << " :: string_array (" << ind << ") size " << str_arr->size() << std::endl;
    if(arr == NULL || str_arr->empty())
        return 0;

    int i=0;
    char *s_str;

    for(std::string s : *str_arr) {
        strcpy(arr[i].s, s.c_str());
        arr[i].len = s.size()+1;
        i++;
    }

    return str_arr->size();
}

void MQLCALL r_int_array_set(int* arr, int size)
{
    if(size > 0)
        std::cerr << " :: int_array_set " << size << ", " << arr[0] << std::endl;
    else
        std::cerr << " :: int_array_set " << size << std::endl;

    int_array.resize(size);
    if(size > 0)
        std::copy(arr, arr+size, int_array.begin());
}

void MQLCALL r_double_array_set(double* arr, int size)
{
    if(size > 0)
        std::cerr << " :: double_array_set " << size << ", " << arr[0] << std::endl;
    else
        std::cerr << " :: double_array_set " << size << std::endl;

    double_array.resize(size);
    if(size > 0)
        std::copy(arr, arr+size, double_array.begin());
}

void MQLCALL r_string_array_set(mql_string *arr, int size)
{
    if(size > 0)
        std::cerr << " :: string_array_set " << size << ", " << arr[0].len << ' ' << std::string(arr[0].s, arr[0].len) << std::endl;
    else
        std::cerr << " :: string_array_set " << size << std::endl;

    string_array.clear();

    for(int i=0; i<size; i++) {
        string_array.push_back(std::string(arr[i].s));
    }
}

int MQLCALL r_recv_pack(zmq::socket_t* c) {
    int r, id, j, ints, doubles, strings, slen;
    unsigned short len;

    std::cerr << " :: r_recv_pack " << c << std::endl;

    try {
        zmq::message_t msg;
        c->recv(&msg);
        if(msg.size() == 0) {
            zmq::message_t empty(0);
            c->send(empty);
            return -1;
        }
        std::string request = static_cast<char*>(msg.data());

        msgpack::unpacked pack;
        msgpack::unpack(&pack, request.data(), request.size());
        msgpack::object result = pack.get();
        std::vector<msgpack::object> result_arr;
        std::vector<int> tmp_int;

        result.convert(&result_arr);
        result_arr[0].convert(&tmp_int);

        if(!tmp_int.empty() && tmp_int[0] > 500) {
            auto ind = reinterpret_cast<Indicator*>((void*)tmp_int[1]);
            if(!ind || std::find(indicators.begin(), indicators.end(), ind) == indicators.end())
                throw -1;
            // TODO: pass information that indicator is closed

            std::cerr << " :: indicator " << ind << std::endl;

            result_arr[0].convert(&ind->ints);
            result_arr[1].convert(&ind->doubles);
            result_arr[2].convert(&ind->strings);
            return 1;
        }

        result_arr[0].convert(&int_array);
        result_arr[1].convert(&double_array);
        result_arr[2].convert(&string_array);

        std::cerr << " :: msgpack::unpacked " << result << std::endl;

        return 0;
    }
    catch(std::bad_alloc&) {
        std::cerr << "recv error: alloc failed" << std::endl;
        return -2;
    }
    catch(msgpack::type_error&) {
        std::cerr << "msgpack error: bad cast" << std::endl;
        return -3;
    }
    catch(...) {
        std::cerr << "recv packet: error" << std::endl;
        return -4;
    }
    return -1;
}

Indicator* MQLCALL ind_init(char *name, char *symbol, int period)
{
    auto ind = new Indicator(std::string(name), std::string(symbol), period);
    indicators.push_back(ind);
    std::cerr << " :: ind :: new " << std::string(name) << " indicator " << indicators.size()-1 << std::endl;
    return ind;
}

int MQLCALL ind_get_all(mql_string *arr)
{
    int i = 0;

    for(auto ind_ptr : indicators) {
        strcpy(arr[i].s, ind_ptr->name.c_str());
        arr[i].len = ind_ptr->name.size()+1;
        i++;
    }

    return indicators.size();
}

int MQLCALL ind_find(char *name, unsigned int *arr, mql_string *str_arr)
{
    int j = 0;
    auto s = std::string(name);

    for(auto ind : indicators) {
        if(ind->name == s) {
            arr[j*2] = reinterpret_cast<unsigned int>(ind);
            arr[j*2+1] = ind->period;
            strcpy(str_arr[j].s, ind->symbol.c_str());
            str_arr[j].len = ind->symbol.size() + 1;

            j++;
        }
    }

    return j;
}

void MQLCALL ind_finish(Indicator *ind)
{
    auto it = std::find(indicators.begin(), indicators.end(), ind);
    if(it == indicators.end()) {
        std::cerr << " :: INVALID DELETE @" << ind << std::endl;
        return;
    }

    delete *it;
    indicators.erase(it);
}
