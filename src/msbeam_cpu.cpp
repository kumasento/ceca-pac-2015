
#include <stdio.h>
#include <omp.h>

#include "wray.h"
#include "proj.h"
#include "proto.h"
#include "utility.h"
#include "msbeam_cpu.h"

namespace MSBeamCpuHelper {

    void min_image(float *f, float *v, float *g, int np, int nr, int *line, 
        float *weight, int numb, float snorm, float lambda) {
        
        int i;
        float d[IMGSIZE*2+10];
        float Af = 0.;

        for (i = 0; i<numb; ++i) {
            int ind = line[i];
            Af += f[ind]*weight[i];
        }
        Af -= g[np*NRAY+nr];
        
        /* calculate div (v^2 \grad f) and Af-g*/
        for (i = 0; i<numb; ++i) { 
            int ind = line[i];
            int x = ind/IMGSIZE, y = ind%IMGSIZE;

            float tmp = 0.;
            float lap = 0.;
            
            if (x+1<IMGSIZE) tmp += sqr(v[ind])*(f[ind+IMGSIZE]-f[ind]);
            else             tmp += sqr(v[ind])*(   0          -f[ind]);
            
            if (y+1<IMGSIZE) tmp += sqr(v[ind])*(f[ind+1]-f[ind]);
            else             tmp += sqr(v[ind])*(   0    -f[ind]);
            
            if (x-1>=0)      tmp -= sqr(v[ind-IMGSIZE])*(f[ind]-f[ind-IMGSIZE]);
            else             tmp -=                     (f[ind]-0        );
            
            if (y-1>=0)      tmp -= sqr(v[ind-1])*(f[ind]-f[ind-1]);
            else             tmp -=               (f[ind]-0       );
            
            if (x+1<IMGSIZE) lap += f[ind+IMGSIZE];
            if (y+1<IMGSIZE) lap += f[ind+1];
            if (x-1>=0)      lap += f[ind-IMGSIZE];
            if (y-1>=0)      lap += f[ind-1];
            lap -= 4*f[ind];

            d[i] = -Af*weight[i]+ALPHA*(tmp+sqr(EPSILON)*lap);
        }
            
        for (i = 0; i<numb; ++i) {
            int ind = line[i];
            f[ind] += lambda*d[i];
            if (f[ind]<0) f[ind] = 0;
            if (f[ind]>255) f[ind] = 255;
        }
    }

    void min_edge(float *f, float *v, int np, int nr, int *line, 
        float *weight, int numb, float lambda) {
        int i;

        float d[IMGSIZE*2+10];
        
        for (i = 0; i<numb; ++i) {
            int ind = line[i];
            int x = ind/IMGSIZE, y = ind%IMGSIZE;
            
            float a = 0.;
            float b = 0.;
            float c = 0.;

            if (x-1>=0)      a += sqr(f[ind]-f[ind-IMGSIZE]);
            else             a += sqr(f[ind]-0        );
            
            if (y-1>=0)      a += sqr(f[ind]-f[ind-1]);
            else             a += sqr(f[ind]-0       );
            
            a *= v[ind];
            
            b = v[ind]-1;
                    
            if (x+1<IMGSIZE) c += v[ind+IMGSIZE];
            if (y+1<IMGSIZE) c += v[ind+1];
            if (x-1>=0)      c += v[ind-IMGSIZE];
            if (y-1>=0)      c += v[ind-1];
            c -= 4*v[ind];
            
            d[i] = -ALPHA*a-BETA/(4*EPSILON)*b+BETA*EPSILON*c;
        }
        
        for (i = 0; i<numb; ++i) {
            int ind = line[i];
            v[ind] += lambda*d[i];
            if (v[ind]<0) v[ind] = 0;
            if (v[ind]>1) v[ind] = 1;
        }
    }

    void min_wrapper(float *f, float *v, float *g, int np, int nr, float lambda) {
        int line[IMGSIZE*2+10];
        float weight[IMGSIZE*2+10];
        int numb;
        float snorm;
        
        wray(np, nr, line, weight, &numb, &snorm);
        
        min_image(f, v, g, np, nr, line, weight, numb, snorm, lambda);
        min_edge(f, v, np, nr, line, weight, numb, lambda); 
    }
}

void MSBeamCpu::msbeam(float *f, float *v, float *g, int num_thread) {
    int i,j,k;
    for (i = 0; i<IMGSIZE*IMGSIZE; ++i) {
        f[i] = 0.;
        v[i] = 1.;
    }
    printf("Image and edge data has been initialized.\n");
    
    float lambda = 0.001;

    printf("Begin msbeam minimization ...\n");
    for (i = 1; i<=ALL_ITER; ++i) {
        printf("\033[0;31mIteration\033[0m %d ...\n", i);
        printf("lambda = %f\n",lambda);
        
        #pragma omp parallel for private(j,k) num_threads(num_thread)
        for (j = 0; j < NPROJ; ++j)
            for (k = 0; k < NRAY; ++k) 
                MSBeamCpuHelper::min_wrapper(f, v, g, j, k, lambda);
        
        lambda = lambda/(1 + 500 * lambda);
    }
    printf("msbeam minimization done.\n");
}
