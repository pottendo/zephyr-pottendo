#include <string>
#include <iostream>
#include <vector>
#include "c64-lib.h"

using namespace std;

class CoRoutine_t {
protected:    
    string name;
public:
    CoRoutine_t(string n) : name(n)
    {
        cout << "CoRoutine: " << *this << '\n';
    }
    ~CoRoutine_t() = default;

    int run(void) { 
        cout << *this << "'s run function called\n";
        return _run();
    }

    virtual int _run(void) = 0;
    friend ostream &operator<<(ostream &ostr, CoRoutine_t &cr) { return ostr << cr.name; }
};

template <typename Paramstruct> 
class CoRoutine : public CoRoutine_t {
    Paramstruct *p;
    c64 &c64i;
public:
    CoRoutine(string n, c64 &_c64) : CoRoutine_t(n), c64i(_c64) { p = (Paramstruct *)_c64.get_coprocreq(); cout << "param = " << p << '\n';};
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

class oc_coproc {
    enum { CNOP = 0, CLINE = 1, CCIRCLE = 2, CEXIT = 0xff };
    c64 &c64i;
    string name;
    vector<CoRoutine_t *> oc_crs;
public:
    oc_coproc(c64 &_c64, string n = "Orangecart");
    ~oc_coproc() = default;

    int loop(void);
};

