#include <unistd.h>
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
    oc_crs.push_back(new CoRoutine<char *>{"nop", _c64});
    oc_crs[CLINE] = new CoRoutine<cr_line_t>{"line", _c64};
    oc_crs[CCIRCLE] = new CoRoutine<cr_circle_t>{"circle", _c64};
    oc_crs[CEXIT] = new CoRoutine<char *>{"exit", _c64};
}

int
oc_coproc::loop(void)
{
    bool leave = false;
    coroutine_t *ctr_reg = (coroutine_t *)c64i.get_coprocreq();
    char *t = (char *)ctr_reg;
    std::cout << name << " is waiting for CoRoutine Requests...\n";
    CACHE_FLUSH();
    while (!leave)
    {
        //printf("ctr_reg(%p): %02x/%02x/%02x/%02x/%02x/%02x/%02x/%02x/%02x\n", t, t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7], t[8]);
        if ((ctr_reg->cmd >2) || !oc_crs[ctr_reg->cmd])
            cout << "not assigned\n";
        else
            leave = oc_crs[ctr_reg->cmd]->run();
        ctr_reg->cmd = CNOP;
        ctr_reg->res = 1;
        //usleep(1000 * 1000);
        CACHE_FLUSH();
    }
    std::cout << __FUNCTION__ << " done.\n";
    return 0;
}

template<>
int CoRoutine<char *>::_run(void)
{
    return 0;
}

template<>
int CoRoutine<cr_line_t>::_run(void)
{
    printf("(%d,%d) -> (%d, %d), col = %d\n", p->x1, p->y1, p->x2, p->y2, p->c);
    // calculate dx & dy
    int X1 = p->x1;
    int Y1 = p->y1;
    int X2 = p->x2;
    int Y2 = p->y2;

    if (X2 < X1) std::swap(X1, X2);
    if (Y2 < Y1) std::swap(Y1, Y2);
    int dx = X2 - X1;
    int dy = Y2 - Y1;
    
    if (dy <= dx)
    {
        // initial value of decision parameter d
        int d = dy - (dx/2);
        int x = X1, y = Y1;
 
        // Plot initial given point
        c64i.setpx(x, y, p->c);
        // iterate through value of X
        while (x < X2)
        {
            x++;
            // E or East is chosen
            if (d < 0)
                d = d + dy;
            else
            {
                // NE or North East is chosen
                d += (dy - dx);
                y++;
            }
            c64i.setpx(x, y, p->c);
        }
    }
    else if (dx<dy)
    {
        // initial value of decision parameter d
        int d = dx - (dy / 2);
        int x = X1, y = Y1;

        // Plot initial given point
        c64i.setpx(x, y, p->c);

        // iterate through value of X
        while (y < Y2)
        {
            y++;
            // E or East is chosen
            if (d < 0)
                d = d + dx;
            else
            {
                // NE or North East is chosen
                d += (dx - dy);
                x++;
            }
            c64i.setpx(x, y, p->c);
        }
    }
    
    return 0;
}

template<>
int CoRoutine<cr_circle_t>::_run(void)
{
    return 0;
}

#if 0
    void line(char *canvas, uint16_t x1, uint8_t y1, uint16_t x2, uint8_t y2, uint8_t c);
    void circle(char *canvas, uint16_t x_centre, uint16_t y_centre, uint16_t r, uint8_t c);

// code reference: https://www.geeksforgeeks.org/mid-point-line-generation-algorithm/
static inline (uint16_t X1, uint8_t Y1, uint16_t X2, uint8_t Y2, uint8_t c)
{
    // calculate dx & dy
   
    if (X2 < X1) std::swap(X1, X2);
    if (Y2 < Y1) std::swap(Y1, Y2);
    int dx = X2 - X1;
    int dy = Y2 - Y1;
    
    if (dy <= dx)
    {
        // initial value of decision parameter d
        int d = dy - (dx/2);
        int x = X1, y = Y1;
 
        // Plot initial given point
        setpx(canvas, x, y, c);
        // iterate through value of X
        while (x < X2)
        {
            x++;
            // E or East is chosen
            if (d < 0)
                d = d + dy;
            else
            {
                // NE or North East is chosen
                d += (dy - dx);
                y++;
            }
            setpx(canvas, x, y, c);
        }
    }
    else if (dx<dy)
    {
        // initial value of decision parameter d
        int d = dx - (dy / 2);
        int x = X1, y = Y1;

        // Plot initial given point
        setpx(canvas, x, y, c);

        // iterate through value of X
        while (y < Y2)
        {
            y++;
            // E or East is chosen
            if (d < 0)
                d = d + dx;
            else
            {
                // NE or North East is chosen
                d += (dx - dy);
                x++;
            }
            setpx(canvas, x, y, c);
        }
    }
}

// code reference: https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm
void c64::circle(char *canvas, uint16_t x_centre, uint16_t y_centre, uint16_t r, uint8_t c)
{
    int x = r, y = 0;
    if (r > IMG_W) return; 
    // Printing the initial point on the axes
    setpx(canvas, x + x_centre, y + y_centre, c);

    // when radius is zero only a single point will be printed
    if (r > 0)
    {
        setpx(canvas, x + x_centre, -y + y_centre, c);
        setpx(canvas, y + x_centre, x + y_centre, c);
        setpx(canvas, -y + x_centre, x + y_centre, c);
    }
     
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
        setpx(canvas, x + x_centre, y + y_centre, c);
        setpx(canvas, -x + x_centre, y + y_centre, c);
        setpx(canvas, x + x_centre, -y + y_centre, c);
        setpx(canvas, -x + x_centre, -y + y_centre, c);
        
        // If the generated point is on the line x = y then
        // the perimeter points have already been printed
        if (x != y)
        {
            setpx(canvas, y + x_centre, x + y_centre, c);
            setpx(canvas, -y + x_centre, x + y_centre, c);
            setpx(canvas, y + x_centre, -x + y_centre, c);
            setpx(canvas, -y + x_centre, -x + y_centre, c);
        }
    }
}
#endif