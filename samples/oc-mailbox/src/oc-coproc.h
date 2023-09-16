#include <time.h>
#include <string>
#include <iostream>
#include <vector>
#include "c64-lib.h"

using namespace std;

class CoRoutine_t {
protected:    
    string name;
    bool show = false;
public:
    CoRoutine_t(string n) : name(n)
    {
        cout << "CoRoutine: " << *this << '\n';
    }
    ~CoRoutine_t() = default;
    /* class private functinos */
    inline void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *result)
    {
    	result->tv_sec  = a->tv_sec  - b->tv_sec;
    	result->tv_nsec = a->tv_nsec - b->tv_nsec;
    	if (result->tv_nsec < 0)
    	{
	        --result->tv_sec;
	        result->tv_nsec += 1000000000L;
	    }
    }

    inline int run(void) { 
        //cout << *this << "'s run function called\n";
        struct timespec tstart, tend, dt;
        int res;

        clock_gettime(CLOCK_REALTIME, &tstart);
        res = _run();
        clock_gettime(CLOCK_REALTIME, &tend);
        timespec_diff(&tend, &tstart, &dt);
        if (show)
            printf("coroutine '%s' done in: %lld.%03lds\n", name.c_str(), dt.tv_sec, dt.tv_nsec / 1000000L);
        return res;
    }

    virtual int _run(void) = 0;
    friend ostream &operator<<(ostream &ostr, CoRoutine_t &cr) { return ostr << cr.name; }
};

template <typename Paramstruct> 
class CoRoutine : public CoRoutine_t {
    Paramstruct *p;
    c64 &c64i;

public:
    CoRoutine(string n, c64 &_c64) : CoRoutine_t(n), c64i(_c64) { p = (Paramstruct *)_c64.get_coprocreq(); };
    ~CoRoutine() = default;

    int _run(void) override;
};

typedef struct __attribute__((packed)) coroutine {
    uint8_t res;
    uint8_t cmd;
} coroutine_t;

typedef struct __attribute__((packed)) cr_line {
    coroutine_t reg;
    uint8_t c;
    uint16_t x1;
    uint8_t y1;
    uint16_t x2;
    uint8_t y2;
} cr_line_t;

typedef struct __attribute__((packed)) cr_circle {
    coroutine_t reg;
    uint8_t c;
    uint16_t x1;
    uint8_t y1;
    uint16_t r;
} cr_circle_t;

typedef struct __attribute__((packed)) cr_circle_el {
    coroutine_t reg;
    uint8_t c;
    uint16_t x1;
    uint16_t y1;
    uint16_t r;
} cr_circle_el_t;

typedef struct __attribute__((packed)) cr_cfg {
    coroutine_t reg;
    uint16_t canvas;
    uint16_t x1;
    uint8_t y1;
    uint16_t x2;
    uint8_t y2;
} cr_cfg_t;

typedef struct __attribute__((packed)) cr_test {
    coroutine_t reg;
    char *test_pattern;
} cr_test_t;

class oc_coproc {
    enum { CSUCC = 0, CNOP = 0, CLINE = 1, CCIRCLE = 2, CCIRCLE_EL = 3, CCFG = 4, CTEST = 0xfe, CEXIT = 0xff };
    c64 &c64i;
    string name;
    vector<CoRoutine_t *> oc_crs;
public:
    oc_coproc(c64 &_c64, string n = "Orangecart");
    ~oc_coproc() = default; // fixme, delete needed

    int loop(void);
    int isr_req(void);
};

