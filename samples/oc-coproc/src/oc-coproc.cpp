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
        usleep(500 * 1000);
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

// code reference: https://www.geeksforgeeks.org/mid-point-line-generation-algorithm/
static void _line(c64 &c64i, int X1, int Y1, int X2, int Y2, uint8_t c)
{
    if (X1 < 0) X1 = 0;
    if (X2 < 0) X2 = 0;
    if (Y1 < 0) Y1 = 0;
    if (Y2 < 0) Y2 = 0;
    if (X1 >= IMG_W / PIXELW) X1 = IMG_W / PIXELW - 1;
    if (X2 >= IMG_W / PIXELW) X2 = IMG_W / PIXELW - 1;
    if (Y1 >= IMG_H) Y1 = IMG_H - 1;
    if (Y2 >= IMG_H) Y2 = IMG_H - 1;
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
        c64i.setpx(x, y, c);
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
            c64i.setpx(x, y, c);
        }
    }
    else if (dx < dy)
    {
        // initial value of decision parameter d
        int d = dx - (dy / 2);
        int x = X1, y = Y1;

        // Plot initial given point
        c64i.setpx(x, y, c);

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
            c64i.setpx(x, y, c);
        }
    }
}

const uint8_t bitml[] = {
    0b11111111,
    0b01111111,
    0b00111111,
    0b00011111,
    0b00001111,
    0b00000111,
    0b00000011,
    0b00000001
};

const uint8_t bitmr[] = {
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111110,
    0b11111111
};

static void _fhline(c64 &c64i, int X1, int Y1, int X2, int Y2, uint8_t c)
{
    //printf("%s: (%d,%d)->(%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);
    if (X1 < 0) X1 = 0;
    if (X2 < 0) X2 = 0;
    if (Y1 < 0) Y1 = 0;
    if (Y2 < 0) Y2 = 0;
    if (X1 >= IMG_W / PIXELW) X1 = IMG_W / PIXELW - 1;
    if (X2 >= IMG_W / PIXELW) X2 = IMG_W / PIXELW - 1;
    if (Y1 >= IMG_H) Y1 = IMG_H - 1;
    if (Y2 >= IMG_H) Y2 = IMG_H - 1;
    if (X2 < X1) std::swap(X1, X2);
    if (Y2 < Y1) std::swap(Y1, Y2);
    //printf("%s: (%d,%d)->(%d,%d), col = %d\n", __FUNCTION__, X1, Y1, X2, Y2, c);

    const uint32_t lineb = IMG_W / 8 * 8; // line 8 bytes per 8x8 pixel
    const int bpb = 8 / PIXELW;
    int x, it, cidx;
    int boffs = (X1 % bpb);
    int boffsx2 = (X2 % bpb);
    unsigned char *cv = c64i.get_canvas();
    unsigned char bcol = (c & 0x3);
    for (int i = 1; i < (8 / PIXELW); i++)
    {
        bcol = (bcol << PIXELW) | (c & 0x3);
    }
    cidx = (Y1 / 8) * lineb + (Y1 % 8) + (X1 / (8 / PIXELW)) * 8;

    //printf("%s: c=%d, X1=%d, X2=%d(%d offs), Y1=%d, boffs=%d, bpb=%d, cidx=0x%04x\n", __FUNCTION__, bcol, X1, X2, (X2%8), Y1, boffs, bpb, cidx);
    x = X1 + (bpb - boffs); // align with next byte
    if (x <= X2)
    {
        cv[cidx] &= ~bitml[boffs];
        cv[cidx] |= (bitml[boffs] & bcol);
        cidx += 8;
    }
    else
    { 
        uint8_t mr=0, ml=0x80, m;
        // just within a byte, fixme MC
        for (int i = boffsx2; i < bpb; i++)
            mr = (ml << 1) | 1;
        for (int i = 0; i < boffs; i++)
            ml = (ml >> 1) | 0x80;
        m = mr | ml;
        cv[cidx] &= ~m;
        cv[cidx] |= (m & bcol);
    }

    for (it = x; (it <= (X2 - bpb)) && (it < IMG_W / PIXELW); it += 8)
    {
        cv[cidx] = bcol;
        cidx += 8;
    }
    if (it < IMG_W / PIXELW)
    {
        cv[cidx] &= ~bitmr[boffsx2];
        cv[cidx] |= (bitmr[boffsx2] & bcol);
    }
}

template<>
int CoRoutine<cr_line_t>::_run(void)
{
    //printf("(%d,%d) -> (%d, %d), col = %d\n", p->x1, p->y1, p->x2, p->y2, p->c);
    _line(c64i, p->x1, p->y1, p->x2, p->y2, p->c);
    
    return 0;
}

static int filled[IMG_H];

// code reference: https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm
template<>
int CoRoutine<cr_circle_t>::_run(void)
{
    //printf("(%d,%d) rad = %d, col = 0x%02x\n", p->x1, p->y1, p->r, p->c);
    if (p->r > IMG_W) return 0; 
    int x_centre = p->x1;
    int y_centre = p->y1;
    int r = p->r;
    int x = r, y = 0;
    bool fill = (p->c & 0x80);
    p->c &= 0xf;    // clear fill flag
    if (fill) memset(filled, 0, IMG_H * sizeof(int));
    // Printing the initial point on the axes
    c64i.setpx(r + x_centre, y_centre, p->c);

    // when radius is zero only a single point will be printed
    if (r > 0)
    {
        c64i.setpx(-r + x_centre, y_centre, p->c);
        if (fill)
        {
            _fhline(c64i, -r + x_centre + 1, y_centre, r + x_centre, y_centre, p->c);
            if ((y_centre < IMG_H) && (y_centre >= 0)) 
                filled[y_centre]++;
        }
        else
        {
            c64i.setpx(x_centre, r + y_centre, p->c);
            c64i.setpx(x_centre, -r + y_centre, p->c);
        }
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
        if (fill)
        {
            int yh1 = y + y_centre;
            int yh2 = -y + y_centre;
            if ((yh1 < IMG_H) && (yh1 >= 0) && !filled[yh1])
            {
                _fhline(c64i, -x + x_centre, yh1, x + x_centre, yh1, p->c);
                filled[yh1]++;
            }
            if ((yh2 < IMG_H) && (yh2 >= 0) && !filled[yh2])
            {
                _fhline(c64i, -x + x_centre, yh2, x + x_centre, yh2, p->c);
                filled[yh2]++;
            }
       }    
        else
        {
            c64i.setpx(x + x_centre, y + y_centre, p->c);
            c64i.setpx(-x + x_centre, y + y_centre, p->c);
            c64i.setpx(x + x_centre, -y + y_centre, p->c);
            c64i.setpx(-x + x_centre, -y + y_centre, p->c);
        }
        // If the generated point is on the line x = y then
        // the perimeter points have already been printed
        if (x != y)
        {
            if (fill)
            {
                int yh1 = x + y_centre;
                int yh2 = -x + y_centre;
                if ((yh1 < IMG_H) && (yh1 >= 0) && !filled[yh1])
                {
                    _fhline(c64i, -y + x_centre, yh1, y + x_centre, yh1, p->c);
                    filled[yh1]++;
                }
                if ((yh2 < IMG_H) && (yh2 >= 0) && !filled[yh2]) 
                {
                    _fhline(c64i, -y + x_centre, yh2, y + x_centre, yh2, p->c);
                    filled[yh2]++;
                }

            }    
            else
            {
                c64i.setpx(y + x_centre, x + y_centre, p->c);
                c64i.setpx(-y + x_centre, x + y_centre, p->c);
                c64i.setpx(y + x_centre, -x + y_centre, p->c);
                c64i.setpx(-y + x_centre, -x + y_centre, p->c);
            }
        }
    }

    return 0;
}
