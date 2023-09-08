#include "oc-coproc.h"
#include "c64-lib.h"
#ifdef ZEPHYR
#include <pthread.h>
#define usleep(m) k_sleep(K_USEC(m))
#endif

#ifdef ZEPHYR
#define CACHE_FLUSH() __asm__ __volatile__(".word 0x500F")
#else
#define CACHE_FLUSH()
#endif

oc_coproc::oc_coproc(c64 &_c64, string n) : c64i(_c64), name(n)
{
    oc_crs = vector<CoRoutine_t *>(0x100, nullptr);
    oc_crs[CNOP] = new CoRoutine<char *>{"nop", _c64};
    oc_crs[CLINE] = new CoRoutine<cr_line_t>{"line", _c64};
    oc_crs[CCIRCLE] = new CoRoutine<cr_circle_t>{"circle", _c64};
    oc_crs[CCIRCLE_EL] = new CoRoutine<cr_circle_el_t>{"circle_el", _c64};
    oc_crs[CCFG] = new CoRoutine<cr_cfg_t>{"config", _c64};
    oc_crs[CEXIT] = new CoRoutine<char *>{"exit", _c64};
    cout << "size allocated: " << oc_crs.size() << " [6] = '" << oc_crs[6] << "'\n";
}

int
oc_coproc::loop(void)
{
    bool leave = false;
    int no = 0;
    coroutine_t *ctr_reg = (coroutine_t *)c64i.get_coprocreq();
    char *t = (char *)ctr_reg;
    std::cout << name << " is waiting for CoRoutine Requests...\n";
    CACHE_FLUSH();
    while (!leave)
    {
        //printf("ctr_reg(%p): %02x/%02x/%02x/%02x/%02x/%02x/%02x/%02x/%02x\n", t, t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7], t[8]);
        if (!oc_crs[ctr_reg->cmd])
        {
            cout << "not assigned: " << ++no << '\n';
            usleep(1000 * 1000);
        }
        else
        {
            switch(oc_crs[ctr_reg->cmd]->run()) 
            {
                case 0xfe: 
                    break; // CNOP don't do anything
                case 0xff:
                    cout << "exit called\n";
                    leave = true;
                default: // some function success
                    ctr_reg->cmd = CNOP;
                    ctr_reg->res = 1;
                    break;
            }
        }
        //usleep(50 * 1000);
        usleep(10);
        CACHE_FLUSH();
    }
    std::cout << __FUNCTION__ << " done.\n";
    return 0;
}

template<>
int CoRoutine<char *>::_run(void)
{
    return 0xfe;
}

template<>
int CoRoutine<cr_line_t>::_run(void)
{
    //show = true;
    //printf("(%d,%d) -> (%d, %d), col = %d\n", p->x1, p->y1, p->x2, p->y2, p->c);
    return c64i.line(p->x1, p->y1, p->x2, p->y2, p->c);
}


template<>
int CoRoutine<cr_circle_t>::_run(void)
{
    //show = true;
    return c64i.circle(p->x1, p->y1, p->r, p->c);
}

// 16 bit for Y and radius (used by Elite harmless)
template<>
int CoRoutine<cr_circle_el_t>::_run(void)
{
    //show = true;
    //printf("%s: (%d,%d), r= %d, col = 0x%02x\n", __FUNCTION__, p->x1, p->y1, p->r, p->c);
    return c64i.circle(p->x1, p->y1, p->r, p->c);
}

template<>
int CoRoutine<cr_cfg_t>::_run(void)
{
    c64i.set_canvas(p->canvas);
    c64i.set_viewport(p->x1, p->y1, p->x2, p->y2);
    show = true;
    printf("%s: canvas = 0x%04x, vp = { (%d,%d),(%d,%d) }\n", __FUNCTION__, p->canvas, p->x1, p->y1, p->x2, p->y2);
    return 0;
}

#if 0
// vertical fill is slower as more bit-ops are needed compared
// to horizontal fill, where full bytes are written to memory
static int filled[IMG_W];

// code reference: https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm
template<>
int CoRoutine<cr_circle_t>::_run(void)
{
    show = true;
    //printf("(%d,%d) rad = %d, col = 0x%02x\n", p->x1, p->y1, p->r, p->c);
    if (p->r > IMG_W) return 0; 
    int x_centre = p->x1;
    int y_centre = p->y1;
    int r = p->r;
    int x = r, y = 0;
    bool fill = (p->c & 0x80);
    p->c &= 0xf;    // clear fill flag
    if (fill) memset(filled, 0, IMG_W * sizeof(int));
    // Printing the initial point on the axes
    c64i.setpx(r + x_centre, y_centre, p->c);

    // when radius is zero only a single point will be printed
    if (r > 0)
    {
        c64i.setpx(-r + x_centre, y_centre, p->c);
        if (fill)
        {
            if ((x_centre < IMG_W) && (x_centre >= 0)) 
            {
                filled[x_centre]++;
                _fvline(c64i, x_centre, -r + y_centre, x_centre, r + y_centre, p->c);
            }
        }
        else
        {
            c64i.setpx(x_centre, -r + y_centre, p->c);
            c64i.setpx(x_centre, r + y_centre, p->c);
        }
    }
    else
        return 0;
     
    // Initialising the value of P
    int P = 1 - r;
    while (x > y)
    {
        y++;
         
        // Mid-point is inside or on the perimeter
        if (P <= 0)
            P = P + 2*y + 1;
        else
        {
            // Mid-point is outside the perimeter
            x--;
            P = P + 2*y - 2*x + 1;
        }
         
        // All the perimeter points have already been printed
        if (x < y)
            break;
         
        // Printing the generated point and its reflection
        // in the other octants after translation
        if (fill)
        {
            int xh1 = x + x_centre;
            int xh2 = -x + x_centre;
            if ((xh1 < IMG_W) && (xh1 >= 0) && !filled[xh1])
            {
                _fvline(c64i, xh1, -y + y_centre, xh1, y + y_centre, p->c);
                filled[xh1]++;
            }
            if ((xh2 < IMG_W) && (xh2 >= 0) && !filled[xh2])
            {
                _fvline(c64i, xh2, -y + y_centre, xh2, y + y_centre, p->c);
                filled[xh2]++;
            }
        }    
        else
        {
            c64i.setpx(x + x_centre, y + y_centre, p->c);
            c64i.setpx(x + x_centre, -y + y_centre, p->c);
            c64i.setpx(-x + x_centre, y + y_centre, p->c);
            c64i.setpx(-x + x_centre, -y + y_centre, p->c);
        }
        // If the generated point is on the line x = y then
        // the perimeter points have already been printed
        if (x != y)
        {
            if (fill)
            {
                int xh1 = y + x_centre;
                int xh2 = -y + x_centre;
                if ((xh1 < IMG_W) && (xh1 >= 0) && !filled[xh1])
                {
                    _fvline(c64i, xh1, -x + y_centre, xh1, x + y_centre, p->c);
                    filled[xh1]++;
                }
                if ((xh2 < IMG_W) && (xh2 >= 0) && !filled[xh2]) 
                {
                    _fvline(c64i, xh2, -x + y_centre, xh2, x + y_centre, p->c);
                    filled[xh2]++;
                }

            }    
            else
            {
                c64i.setpx(y + x_centre, x + y_centre, p->c);
                c64i.setpx(y + x_centre, -x + y_centre, p->c);
                c64i.setpx(-y + x_centre, x + y_centre, p->c);
                c64i.setpx(-y + x_centre, -x + y_centre, p->c);
            }
        }
    }

    return 0;
}
#endif
