/*
Copyright 2005-2015 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* this should be before all X11 .h files */
#include <xorg-server.h>
#include <xorgVersion.h>

/* all driver need this */
#include <xf86.h>
#include <xf86_OSproc.h>

#include "rdp.h"
#include "rdpDraw.h"
#include "rdpClientCon.h"
#include "rdpReg.h"
#include "rdpCopyPlane.h"

#define LOG_LEVEL 1
#define LLOGLN(_level, _args) \
    do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

/******************************************************************************/
static RegionPtr
rdpCopyPlaneOrg(DrawablePtr pSrc, DrawablePtr pDst,
                GCPtr pGC, int srcx, int srcy, int w, int h,
                int dstx, int dsty, unsigned long bitPlane)
{
    GC_OP_VARS;
    RegionPtr rv;

    GC_OP_PROLOGUE(pGC);
    rv = pGC->ops->CopyPlane(pSrc, pDst, pGC, srcx, srcy,
                             w, h, dstx, dsty, bitPlane);
    GC_OP_EPILOGUE(pGC);
    return rv;
}

/******************************************************************************/
RegionPtr
rdpCopyPlane(DrawablePtr pSrc, DrawablePtr pDst,
             GCPtr pGC, int srcx, int srcy, int w, int h,
             int dstx, int dsty, unsigned long bitPlane)
{
    RegionPtr rv;
    rdpPtr dev;
    RegionRec clip_reg;
    RegionRec reg;
    int cd;
    BoxRec box;

    LLOGLN(10, ("rdpCopyPlane:"));
    dev = rdpGetDevFromScreen(pGC->pScreen);
    dev->counts.rdpCopyPlaneCallCount++;
    box.x1 = pDst->x + dstx;
    box.y1 = pDst->y + dsty;
    box.x2 = box.x1 + w;
    box.y2 = box.x1 + h;
    rdpRegionInit(&reg, &box, 0);
    rdpRegionInit(&clip_reg, NullBox, 0);
    cd = rdpDrawGetClip(dev, &clip_reg, pDst, pGC);
    LLOGLN(10, ("rdpCopyPlane: cd %d", cd));
    if (cd == XRDP_CD_CLIP)
    {
        rdpRegionIntersect(&reg, &clip_reg, &reg);
    }
    /* do original call */
    rv = rdpCopyPlaneOrg(pSrc, pDst, pGC, srcx, srcy, w, h,
                         dstx, dsty, bitPlane);
    if (cd != XRDP_CD_NODRAW)
    {
        rdpClientConAddAllReg(dev, &reg, pDst);
    }
    rdpRegionUninit(&clip_reg);
    rdpRegionUninit(&reg);
    return rv;
}
