/*******************************************************************************/
/*  © Université Lille 1, The Pip Development Team (2015-2017)                 */
/*                                                                             */
/*  This software is a computer program whose purpose is to run a minimal,     */
/*  hypervisor relying on proven properties such as memory isolation.          */
/*                                                                             */
/*  This software is governed by the CeCILL license under French law and       */
/*  abiding by the rules of distribution of free software.  You can  use,      */
/*  modify and/ or redistribute the software under the terms of the CeCILL     */
/*  license as circulated by CEA, CNRS and INRIA at the following URL          */
/*  "http://www.cecill.info".                                                  */
/*                                                                             */
/*  As a counterpart to the access to the source code and  rights to copy,     */
/*  modify and redistribute granted by the license, users are provided only    */
/*  with a limited warranty  and the software's author,  the holder of the     */
/*  economic rights,  and the successive licensors  have only  limited         */
/*  liability.                                                                 */
/*                                                                             */
/*  In this respect, the user's attention is drawn to the risks associated     */
/*  with loading,  using,  modifying and/or developing or reproducing the      */
/*  software by the user in light of its specific status of free software,     */
/*  that may mean  that it is complicated to manipulate,  and  that  also      */
/*  therefore means  that it is reserved for developers  and  experienced      */
/*  professionals having in-depth computer knowledge. Users are therefore      */
/*  encouraged to load and test the software's suitability as regards their    */
/*  requirements in conditions enabling the security of their systems and/or   */
/*  data to be ensured and,  more generally, to use and operate it in the      */
/*  same conditions as regards security.                                       */
/*                                                                             */
/*  The fact that you are presently reading this means that you have had       */
/*  knowledge of the CeCILL license and that you accept its terms.             */
/*******************************************************************************/

#include <stdint.h>
#include <pip/fpinfo.h>
#include <pip/debug.h>
#include <pip/api.h>

/* Defined in mandelbrot.c */
extern void mandelbrot(int, int);

/* Defined in boot.s */
extern void apboot();

/* Weird, unoptimized and potentially broken sync barrier */
uint32_t finished_cores = 0;

/* Read the low part of RDTSC */
/* NOTE :
 * RDTSC returns the CPU cycle count as a 64 bits integer in EDX:EAX.
 * Here, we only fetch EAX, which is the lower 32 bits.
 * We "hope" EDX won't increment during the bench, thus for various reasons we ran the bench several times until
 * we effectively had a coherent value into EDX:EAX.
 * A correct implementation would also check for EDX...
 */
int32_t rdtsc_low()
{
    int32_t eax, edx;
    __asm volatile("RDTSC; MOV %%EAX, %0; MOV %%EDX, %1" : "=r"(eax), "=r"(edx) :: "eax", "edx");
    return eax;
}

/* AP entrypoint - test */
void ap_main(uint32_t data1, uint32_t data2)
{
    /* Get core ID and core count */
    uint32_t cid = Pip_SmpRequest(0, 0);
    uint32_t cc = Pip_SmpRequest(1, 0);

    /* Perform Mandelbrot on slice */
    mandelbrot(cid, cc);

    /* Increment amoutn of finished cores */
    finished_cores++;
    for(;;);
}

void main(pip_fpinfo* bootinfo)
{
    /* Get core ID and core count */
    uint32_t coreid, corecount;
    coreid = Pip_SmpRequest(0, 0);
    corecount = Pip_SmpRequest(1, 0);

    /* Setup each available AP */
    unsigned int core;
    for(core = 1; core < corecount; core++)
    {
        Pip_Debug_Puts("Setting up core ");
        Pip_Debug_PutDec(core);
        Pip_Debug_Puts("\n");
        uint32_t* ap_ep = (uint32_t*)(0xFFFFF000 - core * 0x1000); /* Find VIDT */
        *ap_ep = (uint32_t)&apboot;
        *(ap_ep + 1) = 0x3000000 - core * 0x4000; /* Some randomly placed stack, who cares right now */
    }

    /* Read RDTSC - see rdtsc_low's implementation header for more details about what should really be cared for here */
    int32_t eax_st = rdtsc_low();
    Pip_Debug_Puts("TSC low: ");
    Pip_Debug_PutDec(eax_st);
    Pip_Debug_Puts("\n");

    /* Cores initialized. Bootup cores and run mandelbrot */ 
    for(core = 1; core < corecount; core++)
        Pip_Dispatch(0x0, 0x0, core, 0x42, 0x87);
    
    mandelbrot(coreid, corecount);
    finished_cores++;
    while(finished_cores < corecount);

    int32_t eax_end = rdtsc_low();
    Pip_Debug_Puts("-> Computation time : ");
    Pip_Debug_PutDec(eax_end - eax_st);
    Pip_Debug_Puts("\n");

    for(;;);
}  
