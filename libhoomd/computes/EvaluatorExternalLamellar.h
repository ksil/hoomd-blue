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

// $Id: EvaluatorConstraintSphere.h 3289 2010-08-03 18:09:19Z joaander $
// $URL: http://codeblue.umich.edu/hoomd-blue/svn/trunk/libhoomd/computes/EvaluatorConstraintSphere.h $
// Maintainer: joaander

#ifndef __EVALUATOR_EXTERNAL_LAMELLAR_H__
#define __EVALUATOR_EXTERNAL_LAMELLAR_H__

#include "HOOMDMath.h"

/*! \file EvaluatorConstraintSphere.h
    \brief Defines the constraint evaluator class for spheres
*/

// need to declare these class methods with __device__ qualifiers when building in nvcc
//! DEVICE is __host__ __device__ when included in nvcc and blank when included into the host compiler
#ifdef NVCC
#define DEVICE __device__
#else
#define DEVICE
#endif

// call different optimized sqrt functions on the host / device
//! RSQRT is rsqrtf when included in nvcc and 1.0 / sqrt(x) when included into the host compiler

const Scalar pi=Scalar(3.141592653589793238462643383279502884197169399375105820974944);
//! Class for evaluating sphere constraints
/*! <b>General Overview</b>
    EvaluatorConstraintSphere is a low level computation helper class to aid in evaluating particle constraints on a
    sphere. Given a sphere at a given position and radius, it will find the nearest point on the sphere to a given
    point.
*/
class EvaluatorExternalLamellar
    {
    public:

        typedef Scalar4 param_type;

        //! Constructs the constraint evaluator
        /*! \param X position of particle
            \param Lx length of simulation box in x direction
            \param Ly length of simulation box in y direction
            \param Lz length of simulation box in z direction
            \param params per-type parameters of external potential
        */
        DEVICE EvaluatorExternalLamellar(Scalar3 X, const Scalar Lx, const Scalar Ly, const Scalar Lz, const param_type& params)
            : m_pos(X),
              m_Lx(Lx),
              m_Ly(Ly),
              m_Lz(Lz),
              m_index(__scalar_as_int(params.x)),
              m_orderParameter(params.y),
              m_interfaceWidth(params.z),
              m_periodicity(__scalar_as_int(params.w))
            {
            }

        //! Evaluate the force, energy and virial
        /*! \param F force vector
            \param energy value of the energy
            \param virial array of six scalars for the symmetrized virial tensor
        */
        DEVICE void evalForceEnergyAndVirial(Scalar3& F, Scalar& energy, Scalar* virial)
            {
            Scalar d = Scalar(0.0);
            Scalar perpLength = Scalar(0.0);

            F.x = Scalar(0.0);
            F.y = Scalar(0.0);
            F.z = Scalar(0.0);
            energy = Scalar(0.0);

            // For this potential, since it uses scaled positions, the virial is always zero.
            for (unsigned int i = 0; i < 6; i++)
                virial[i] = Scalar(0.0);

            // compute the vector pointing from P to V
            if (m_index == 0)
                {
                d = m_pos.x;
                perpLength = m_Lx;
                }
            else if (m_index == 1)
                {
                d = m_pos.y;
                perpLength = m_Ly;
                }
            else if (m_index == 2)
                {
                d = m_pos.z;
                perpLength = m_Lz;
                }

            Scalar q, clipParameter, arg, clipcos, tanH, sechSq;

            q = (Scalar(2.0)*pi*m_periodicity)/perpLength;
            clipParameter   = Scalar(1.0)/(q*(m_interfaceWidth*perpLength));
            arg = q*d;
            clipcos = clipParameter*cosf(arg);
            tanH = tanhf(clipcos);
            sechSq = (Scalar(1.0) - tanH*tanH);

            Scalar force = Scalar(0.0);
            force = m_orderParameter*sechSq*clipParameter*sinf(arg)*q;
            energy = m_orderParameter*tanH;

            if (m_index == 0)
                F.x = force;
            else if (m_index == 1)
                F.y = force;
            else if (m_index == 2)
                F.z = force;

            }

    protected:
        Scalar3 m_pos;
        Scalar m_Lx;
        Scalar m_Ly;
        Scalar m_Lz;
        unsigned int m_index;
        Scalar m_orderParameter;
        Scalar m_interfaceWidth;
        unsigned int m_periodicity;
   };


#endif // __EVALUATOR_EXTERNAL_LAMELLAR_H__

