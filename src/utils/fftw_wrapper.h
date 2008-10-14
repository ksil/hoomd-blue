/*
Highly Optimized Object-Oriented Molecular Dynamics (HOOMD) Open
Source Software License
Copyright (c) 2008 Ames Laboratory Iowa State University
All rights reserved.

Redistribution and use of HOOMD, in source and binary forms, with or
without modification, are permitted, provided that the following
conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names HOOMD's
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND
CONTRIBUTORS ``AS IS''  AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __FFTW__WRAPPER__
#define __FFTW__WRAPPER__

/*! \file fftw_wrapper.h
	\brief Adapts fftw for use in HOOMD
*/

//! fftw for HOOMD
/*! fffw stands for fastest fourier transform in the west, see http://www.fftw.org/ for
    proper documentation and details. 
	The fftw used in this class is done in double precision.
	\note Due to licensing conflicts fftw is not distributed by HOOMD.
*/

#include "fft_class.h"
#include "fftw3.h"

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif

class fftw_wrapper:public fft_class
    {
	public:
			fftw_wrapper();  //!< void constructor
			fftw_wrapper(unsigned int N_x,unsigned int N_y,unsigned int N_z); //!< constructor
			~fftw_wrapper(void); //!< destructor

			void fftw_define(unsigned int N_x,unsigned int N_y,unsigned int N_z); //<! Redefines the class if constructed by void constructor
	           
			void cmplx_fft(unsigned int N_x,unsigned int N_y,unsigned int N_z,CScalar ***Dat_in,CScalar ***Dat_out,int sig); //<! fft transform complex to complex
			void real_to_compl_fft(unsigned int N_x,unsigned int N_y,unsigned int N_z,Scalar ***Dat_in,CScalar ***Dat_out);  //<! fft transform real to complex
			void compl_to_real_fft(unsigned int N_x,unsigned int N_y,unsigned int N_z,CScalar ***Dat_in,Scalar ***Dat_out);  //<! fft transform complex to real

	private:
			unsigned int N_x,N_y,N_z;
			fftw_complex *in_f, *out_f;
			fftw_complex *in_b, *out_b;
			fftw_plan p_forward;
			fftw_plan p_backward;
			bool plan_is_defined;
			/*! \param N_x number of grid points in the x axis
			    \param N_y number of grid points in the y axis
			    \param N_z number of grid points in the z axis
				\param in_f input data for forward fft
				\param out_f output data for forward fft
				\param in_b input data for backward fft
				\param out_b output data for backward fft
				\param p_forward structure defining the forward plan for fft
				\param p_backward structure defining the backward plan for fft
                \param plan_is_defined make sure plan is defined only once
			*/
			double Initial_Conf_real(double x,double y); //<! Defines the real part of an initial configuration
			double Initial_Conf_Imag(double x,double y); //<! Defines the imag part of an initial configuration
			
};
#endif
