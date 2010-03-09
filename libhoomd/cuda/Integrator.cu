/*
Highly Optimized Object-oriented Many-particle Dynamics -- Blue Edition
(HOOMD-blue) Open Source Software License Copyright 2008, 2009 Ames Laboratory
Iowa State University and The Regents of the University of Michigan All rights
reserved.

HOOMD-blue may contain modifications ("Contributions") provided, and to which
copyright is held, by various Contributors who have granted The Regents of the
University of Michigan the right to modify and/or distribute such Contributions.

Redistribution and use of HOOMD-blue, in source and binary forms, with or
without modification, are permitted, provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions, and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions, and the following disclaimer in the documentation and/or
other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of HOOMD-blue's
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND/OR
ANY WARRANTIES THAT THIS SOFTWARE IS FREE OF INFRINGEMENT ARE DISCLAIMED.

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// $Id$
// $URL$
// Maintainer: joaander

#include "Integrator.cuh"
#include "gpu_settings.h"

#ifdef WIN32
#include <cassert>
#else
#include <assert.h>
#endif

/*! \file Integrator.cu
    \brief Defines methods and data structures used by the Integrator class on the GPU
*/

//! helper to add a given force/virial pointer pair
__device__ void add_force_total(float4& net_force, float& net_virial, float4* d_f, float* d_v, int idx)
    {
    if (d_f != NULL && d_v != NULL)
        {
        float4 f = d_f[idx];
        float v = d_v[idx];
        net_force.x += f.x;
        net_force.y += f.y;
        net_force.z += f.z;
        net_force.w += f.w;
        net_virial += v;
        }
    }

//! Kernel for summing forces on the GPU
/*! \param d_net_force Output device array to hold the computed net force
    \param d_net_virial Output device array to hold the computed net virial
    \param force_list List of pointers to force data to sum
    \param nparticles Number of particles in the arrays
    \param clear When true, initializes the sums to 0 before adding. When false, reads in the current \a d_net_force
           and \a d_net_virial and adds to that
*/
__global__ void gpu_integrator_sum_net_force_kernel(float4 *d_net_force,
                                                    float *d_net_virial,
                                                    const gpu_force_list force_list,
                                                    unsigned int nparticles,
                                                    bool clear)
    {
    // calculate the index we will be handling
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    
    if (idx < nparticles)
        {
        // set the initial net_force and net_virial to sum into
        float4 net_force;
        float net_virial;
        
        if (clear)
            {
            net_force = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
            net_virial = 0.0f;
            }
        else
            {
            // if clear is false, intialize to the current d_net_force and d_net_virial
            net_force = d_net_force[idx];
            net_virial = d_net_virial[idx];
            }
        
        // sum up the totals
        add_force_total(net_force, net_virial, force_list.f0, force_list.v0, idx);
        add_force_total(net_force, net_virial, force_list.f1, force_list.v1, idx);
        add_force_total(net_force, net_virial, force_list.f2, force_list.v2, idx);
        add_force_total(net_force, net_virial, force_list.f3, force_list.v3, idx);
        add_force_total(net_force, net_virial, force_list.f4, force_list.v4, idx);
        add_force_total(net_force, net_virial, force_list.f5, force_list.v5, idx);
        
        // write out the final result
        d_net_force[idx] = net_force;
        d_net_virial[idx] = net_virial;
        }
    }

/*! The speicified forces and virials are summed for every particle into \a d_net_force and \a d_net_virial

    \param d_net_force Output device array to hold the computed net force
    \param d_net_virial Output device array to hold the computed net virial
    \param force_list List of pointers to force data to sum
    \param nparticles Number of particles in the arrays
    \param clear When true, initializes the sums to 0 before adding. When false, reads in the current \a d_net_force
           and \a d_net_virial and adds to that

    \returns Any error code from the kernel call retrieved via cudaGetLastError() (if g_gpu_error_checking is true)
*/
cudaError_t gpu_integrator_sum_net_force(float4 *d_net_force,
                                         float *d_net_virial,
                                         const gpu_force_list& force_list,
                                         unsigned int nparticles,
                                         bool clear)
    {
    // sanity check
    assert(d_net_force);
    assert(d_net_virial);
    
    const int block_size = 256;
    
    gpu_integrator_sum_net_force_kernel<<< nparticles/block_size+1, block_size >>>
        (d_net_force, d_net_virial, force_list, nparticles, clear);
    
    if (!g_gpu_error_checking)
        {
        return cudaSuccess;
        }
    else
        {
        cudaThreadSynchronize();
        return cudaGetLastError();
        }
    }
