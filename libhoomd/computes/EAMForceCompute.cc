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

// $Id$
// $URL$
// Maintainer: morozov

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4103 4244 )
#endif

#include <boost/python.hpp>
#include <boost/dynamic_bitset.hpp>
using namespace boost;
using namespace boost::python;

#include "HOOMDInitializer.h"
#include "EAMForceCompute.h"
#include <stdexcept>

/*! \file EAMForceCompute.cc
    \brief Defines the EAMForceCompute class
*/

using namespace std;

/*! \param sysdef System to compute forces on
    \param filename Name of EAM potential file to load
    \param type_of_file Undocumented parameter
*/
EAMForceCompute::EAMForceCompute(boost::shared_ptr<SystemDefinition> sysdef, char *filename, int type_of_file)
    : ForceCompute(sysdef)
    {
    assert(m_pdata);



    loadFile(filename, type_of_file);
    // initialize the number of types value
    m_ntypes = m_pdata->getNTypes();
    assert(m_ntypes > 0);


    }


EAMForceCompute::~EAMForceCompute()
    {

    }
/*
type_of_file = 0 => EAM/Alloy
type_of_file = 1 => EAM/FS
*/
void EAMForceCompute::loadFile(char *filename, int type_of_file)
    {
    unsigned int tmp_int, type, i, j, k;
    double  tmp_mass, tmp;
    char tmp_str[5];

    const unsigned int MAX_TYPE_NUMBER = 10;
    const unsigned int MAX_POINT_NUMBER = 1000000;
    
    // Open potential file
    FILE *fp;
    fp = fopen(filename,"r");
    if (fp == NULL)
        {
        cerr << endl << "***Error! Can not load EAM file" << endl << endl;
        throw runtime_error("Error loading file");
        }
    for(i = 0; i < 3; i++) while(fgetc(fp) != '\n');
    m_ntypes = 0;
    fscanf(fp, "%d", &m_ntypes);
    if(m_ntypes < 1 || m_ntypes > MAX_TYPE_NUMBER ) 
        {
        cerr << endl << "***Error! Invalid EAM file format: Type number is greater than " << MAX_TYPE_NUMBER << endl << endl;
        throw runtime_error("Error loading file");
        }
    // temporary array to count used types
    boost::dynamic_bitset<> types_set(m_pdata->getNTypes()); 
    //Load names of types.
    for(i = 0; i < m_ntypes; i++)
        {
        fscanf(fp, "%2s", tmp_str);
        names.push_back(tmp_str);
        unsigned int tid = m_pdata->getTypeByName(tmp_str);
        types.push_back(tid);
        types_set[tid] = true;
        }

    //Check that all types of atopms in xml file have description in potential file
    if(m_pdata->getNTypes() != types_set.count())
        {
        cerr << endl << "***Error! not all atom types are defined in EAM potential file!!!" << endl << endl;
        throw runtime_error("Error loading file");
        }
/*    for(i = 0; i < m_ntypes; i++){
        cout << i << " : " << types[i] << " " << names[i] << endl;
    }
    */
    //Load parameters.
    fscanf(fp,"%d", &nrho);
    fscanf(fp,"%lg", &tmp);
    drho = tmp;
    rdrho = (Scalar)(1.0 / drho);
    fscanf(fp,"%d", &nr);
    fscanf(fp,"%lg", &tmp);
    dr = tmp;
    rdr = (Scalar)(1.0 / dr);
    fscanf(fp,"%lg", &tmp);
    m_r_cut = tmp;
    if (nrho < 1 || nr < 1 || nrho > MAX_POINT_NUMBER || nr > MAX_POINT_NUMBER)
        {
        cerr << endl << "***Error! Invalid EAM file format: Point number is greater than " << MAX_POINT_NUMBER << endl << endl;
        throw runtime_error("Error loading file");
        }
    //Resize arrays for tables
    embeddingFunction.resize(nrho * m_ntypes);
    electronDensity.resize( nr * m_ntypes * m_ntypes);
    pairPotential.resize( (int)(0.5 * nr * (m_ntypes + 1) * m_ntypes) );
    derivativeEmbeddingFunction.resize(nrho * m_ntypes);
    derivativeElectronDensity.resize(nr * m_ntypes * m_ntypes);
    derivativePairPotential.resize((int)(0.5 * nr * (m_ntypes + 1) * m_ntypes));
    int res = 0;
    for(type = 0 ; type < m_ntypes; type++)
        {
        fscanf(fp, "%d %lg %lg %3s ", &tmp_int, &tmp_mass, &tmp, tmp_str);
        mass.push_back(tmp_mass);

        //Read F's array

        for(i = 0 ; i < nrho; i++)
            {
            res = fscanf(fp, "%lg", &tmp);
            embeddingFunction[types[type] * nrho + i] = (Scalar)tmp;
            }
        //Read Rho's arrays
        //If FS we need read N arrays
        //If Alloy we ned read 1 array, and then duplicate N-1 times.
        unsigned int count = 1;
        if(type_of_file == 1) count = m_ntypes;
        //Read
        for(j = 0; j < count; j++)
            {
            for(i = 0 ; i < nr; i++)
                {
                res = fscanf(fp, "%lg", &tmp);
                electronDensity[types[type] * m_ntypes * nr + j * nr + i] = (Scalar)tmp;
                }
            }

        for(j = 1; j <= m_ntypes - count; j++)
            {
            for(i = 0 ; i < nr; i++)
                {
                electronDensity[types[type] * m_ntypes * nr + j * nr + i] = electronDensity[i];
                }


            }
        }
    
    if(res == EOF || res == 0)
        {
        cerr << endl << "***Error! EAM file is truncated " << endl << endl;
        throw runtime_error("Error loading file");        
        }
    //Read V(r)'s arrays
    for (k = 0; k < m_ntypes; k++)
        {
        for(j = 0; j <= k; j++)
            {
            for(i = 0 ; i < nr; i++)
                {
                res = fscanf(fp, "%lg", &tmp);
                pairPotential[(unsigned int)ceil(0.5 *(2 * m_ntypes - types[k] -1) * types[k] + types[j]) * nr + i].x = (Scalar)tmp;

                }
            }

        }
    
    fclose(fp);





    //Cumpute derivative of Embedding Function and Electron Density.
    for(type = 0 ; type < m_ntypes; type++)
        {
        for(i = 0 ; i < nrho - 1; i++)
            {
            derivativeEmbeddingFunction[i + types[type] * nrho] =
                (embeddingFunction[i + 1 + types[type] * nrho] - embeddingFunction[i + types[type] * nrho]) / drho;
            }
        for(j = 0; j < m_ntypes; j++)
            {
            for(i = 0 ; i < nr - 1; i++)
                {
                derivativeElectronDensity[types[type] * m_ntypes * nr +  j * nr + i ] =
                    (electronDensity[types[type] * m_ntypes * nr +  j * nr + i + 1] -
                    electronDensity[types[type] * m_ntypes * nr +  j * nr + i]) / dr;
                }
            }

        }


    //Cumpute derivative of Pair Potential.

    for (k = 0; k < m_ntypes; k ++)
        {
        for(j = 0; j <= k; j++)
            {
            for(i = 0 ; i < nr; i++)
                {
                if((i + 1)%nr == 0) continue;
                pairPotential[(unsigned int)ceil(0.5 *(2 * m_ntypes - types[k] -1) * types[k])  + types[j] * nr + i].y =
                (pairPotential[(unsigned int)ceil(0.5 *(2 * m_ntypes - types[k] -1) * types[k]) + types[j] * nr + i + 1].x - pairPotential[(unsigned int)ceil(0.5 *(2 * m_ntypes - types[k] -1) * types[k]) + types[j] * nr + i].x) / dr;

                }
            }

        }

    }
std::vector< std::string > EAMForceCompute::getProvidedLogQuantities()
    {
    vector<string> list;
    list.push_back("pair_lj_energy");
    return list;
    }

Scalar EAMForceCompute::getLogValue(const std::string& quantity, unsigned int timestep)
    {
    if (quantity == string("pair_lj_energy"))
        {
        compute(timestep);
        return calcEnergySum();
        }
    else
        {
        cerr << endl << "***Error! " << quantity << " is not a valid log quantity for EAMForceCompute" << endl << endl;
        throw runtime_error("Error getting log value");
        }
    }

/*! \post The lennard jones forces are computed for the given timestep. The neighborlist's
     compute method is called to ensure that it is up to date.

    \param timestep specifies the current time step of the simulation
*/
void EAMForceCompute::computeForces(unsigned int timestep)
    {
    // start by updating the neighborlist
    m_nlist->compute(timestep);

    // start the profile for this compute
    if (m_prof) m_prof->push("EAM pair");

    // depending on the neighborlist settings, we can take advantage of newton's third law
    // to reduce computations at the cost of memory access complexity: set that flag now
    bool third_law = m_nlist->getStorageMode() == NeighborList::half;

    // access the neighbor list
    assert(m_nlist);
    ArrayHandle<unsigned int> h_n_neigh(m_nlist->getNNeighArray(), access_location::host, access_mode::read);
    ArrayHandle<unsigned int> h_nlist(m_nlist->getNListArray(), access_location::host, access_mode::read);
    Index2D nli = m_nlist->getNListIndexer();

    // access the particle data
    const ParticleDataArraysConst& arrays = m_pdata->acquireReadOnly();
    // sanity check
    assert(arrays.x != NULL && arrays.y != NULL && arrays.z != NULL);

    // get a local copy of the simulation box too
    const BoxDim& box = m_pdata->getBox();
    // sanity check
    assert(box.xhi > box.xlo && box.yhi > box.ylo && box.zhi > box.zlo);

    // create a temporary copy of r_cut sqaured
    Scalar r_cut_sq = m_r_cut * m_r_cut;

    // precalculate box lenghts for use in the periodic imaging
    Scalar Lx = box.xhi - box.xlo;
    Scalar Ly = box.yhi - box.ylo;
    Scalar Lz = box.zhi - box.zlo;

    // tally up the number of forces calculated
    int64_t n_calc = 0;

    // need to start from a zero force, energy and virial
    // (MEM TRANSFER: 5*N scalars)
    memset(m_fx, 0, sizeof(Scalar)*arrays.nparticles);
    memset(m_fy, 0, sizeof(Scalar)*arrays.nparticles);
    memset(m_fz, 0, sizeof(Scalar)*arrays.nparticles);
    memset(m_pe, 0, sizeof(Scalar)*arrays.nparticles);
    memset(m_virial, 0, sizeof(Scalar)*arrays.nparticles);

    // for each particle
    vector<Scalar> atomElectronDensity;
    atomElectronDensity.resize(arrays.nparticles);
    vector<Scalar> atomDerivativeEmbeddingFunction;
    atomDerivativeEmbeddingFunction.resize(arrays.nparticles);
    vector<Scalar> atomEmbeddingFunction;
    atomEmbeddingFunction.resize(arrays.nparticles);
    unsigned int ntypes = m_pdata->getNTypes();
    for (unsigned int i = 0; i < arrays.nparticles; i++)
        {
        // access the particle's position and type (MEM TRANSFER: 4 scalars)
        Scalar xi = arrays.x[i];
        Scalar yi = arrays.y[i];
        Scalar zi = arrays.z[i];
        unsigned int typei = arrays.type[i];

        // sanity check
        assert(typei < m_pdata->getNTypes());

        // loop over all of the neighbors of this particle
        const unsigned int size = (unsigned int)h_n_neigh.data[i];

        for (unsigned int j = 0; j < size; j++)
            {
            // increment our calculation counter
            n_calc++;

            // access the index of this neighbor (MEM TRANSFER: 1 scalar)
            unsigned int k = h_nlist.data[nli(i, j)];
            // sanity check
            assert(k < m_pdata->getN());

            // calculate dr (MEM TRANSFER: 3 scalars / FLOPS: 3)
            Scalar dx = xi - arrays.x[k];
            Scalar dy = yi - arrays.y[k];
            Scalar dz = zi - arrays.z[k];

            // access the type of the neighbor particle (MEM TRANSFER: 1 scalar
            unsigned int typej = arrays.type[k];
            // sanity check
            assert(typej < m_pdata->getNTypes());

            // apply periodic boundary conditions (FLOPS: 9 (worst case: first branch is missed, the 2nd is taken and the add is done)
            if (dx >= box.xhi)
                dx -= Lx;
            else
            if (dx < box.xlo)
                dx += Lx;

            if (dy >= box.yhi)
                dy -= Ly;
            else
            if (dy < box.ylo)
                dy += Ly;

            if (dz >= box.zhi)
                dz -= Lz;
            else
            if (dz < box.zlo)
                dz += Lz;

            // start computing the force
            // calculate r squared (FLOPS: 5)
            Scalar rsq = dx*dx + dy*dy + dz*dz;
            // only compute the force if the particles are closer than the cuttoff (FLOPS: 1)
            if (rsq < r_cut_sq)
                {
                 Scalar position_float = sqrt(rsq) * rdr;
                 Scalar position = position_float;
                 unsigned int r_index = (unsigned int)position;
                 r_index = min(r_index,nr);
                 position -= r_index;
                 atomElectronDensity[i] += electronDensity[r_index + nr * (typei * ntypes + typej)] + derivativeElectronDensity[r_index + nr * (typei * ntypes + typej)] * position * dr;
                 if(third_law)
                    {
                    atomElectronDensity[k] += electronDensity[r_index + nr * (typej * ntypes + typei)]
                        + derivativeElectronDensity[r_index + nr * (typej * ntypes + typei)] * position * dr;
                    }
                }
            }
        }

    for (unsigned int i = 0; i < arrays.nparticles; i++)
        {
        unsigned int typei = arrays.type[i];

        Scalar position = atomElectronDensity[i] * rdrho;
        unsigned int r_index = (unsigned int)position;
        r_index = min(r_index,nrho);
        position -= (Scalar)r_index;
        atomDerivativeEmbeddingFunction[i] = derivativeEmbeddingFunction[r_index + typei * nrho];

        m_pe[i] += embeddingFunction[r_index + typei * nrho] + derivativeEmbeddingFunction[r_index + typei * nrho] * position * drho;
        }

    for (unsigned int i = 0; i < arrays.nparticles; i++)
        {
        // access the particle's position and type (MEM TRANSFER: 4 scalars)
        Scalar xi = arrays.x[i];
        Scalar yi = arrays.y[i];
        Scalar zi = arrays.z[i];
        unsigned int typei = arrays.type[i];
        // sanity check
        assert(typei < m_pdata->getNTypes());



        // initialize current particle force, potential energy, and virial to 0
        Scalar fxi = 0.0;
        Scalar fyi = 0.0;
        Scalar fzi = 0.0;
        Scalar pei = 0.0;
        Scalar viriali = 0.0;

        // loop over all of the neighbors of this particle
        const unsigned int size = (unsigned int)h_n_neigh.data[i];
        for (unsigned int j = 0; j < size; j++)
            {
            // increment our calculation counter
            n_calc++;

            // access the index of this neighbor (MEM TRANSFER: 1 scalar)
            unsigned int k = h_nlist.data[nli(i, j)];
            // sanity check
            assert(k < m_pdata->getN());

            // calculate dr (MEM TRANSFER: 3 scalars / FLOPS: 3)
            Scalar dx = xi - arrays.x[k];
            Scalar dy = yi - arrays.y[k];
            Scalar dz = zi - arrays.z[k];

            // access the type of the neighbor particle (MEM TRANSFER: 1 scalar
            unsigned int typej = arrays.type[k];
            // sanity check
            assert(typej < m_pdata->getNTypes());

            // apply periodic boundary conditions (FLOPS: 9 (worst case: first branch is missed, the 2nd is taken and the add is done)
            if (dx >= box.xhi)
                dx -= Lx;
            else
            if (dx < box.xlo)
                dx += Lx;

            if (dy >= box.yhi)
                dy -= Ly;
            else
            if (dy < box.ylo)
                dy += Ly;

            if (dz >= box.zhi)
                dz -= Lz;
            else
            if (dz < box.zlo)
                dz += Lz;

            // start computing the force
            // calculate r squared (FLOPS: 5)
            Scalar rsq = dx*dx + dy*dy + dz*dz;


            if (rsq >= r_cut_sq) continue;
            Scalar r = sqrt(rsq);
            Scalar inverseR = 1.0 / r;
            Scalar position = r * rdr;
            unsigned int r_index = (unsigned int)position;
            position = position - (Scalar)r_index;
            int shift = (typei>=typej)?(int)(0.5 * (2 * ntypes - typej -1)*typej + typei) * nr:(int)(0.5 * (2 * ntypes - typei -1)*typei + typej) * nr;
            //r_index = min(r_index,nr - 1);
            Scalar pair_eng = (pairPotential[r_index + shift].x +
                pairPotential[r_index + shift].y * position * dr) * inverseR;
            Scalar derivativePhi = (pairPotential[r_index + shift].y - pair_eng) * inverseR;
            Scalar derivativeRhoI = derivativeElectronDensity[r_index + typei * nr];
            Scalar derivativeRhoJ = derivativeElectronDensity[r_index + typej * nr];
            Scalar fullDerivativePhi = atomDerivativeEmbeddingFunction[i] * derivativeRhoJ +
                atomDerivativeEmbeddingFunction[k] * derivativeRhoI + derivativePhi;
            Scalar pairForce = - fullDerivativePhi * inverseR;
            viriali += float(1.0/3.0) * (rsq) * pairForce;
            fxi += dx * pairForce;
            fyi += dy * pairForce;
            fzi += dz * pairForce;
            pei += pair_eng;

            if (third_law)
                {
                m_fx[k] -= dx * pairForce;
                m_fy[k] -= dy * pairForce;
                m_fz[k] -= dz * pairForce;
                }
            }
        m_fx[i] += fxi;
        m_fy[i] += fyi;
        m_fz[i] += fzi;
        m_pe[i] += pei;
        m_virial[i] += viriali;
        }
    m_pdata->release();
    #ifdef ENABLE_CUDA
    // the force data is now only up to date on the cpu
    m_data_location = cpu;
    #endif

    int64_t flops = m_pdata->getN() * 5 + n_calc * (3+5+9+1+9+6+8);
    if (third_law) flops += n_calc * 8;
    int64_t mem_transfer = m_pdata->getN() * (5+4+10)*sizeof(Scalar) + n_calc * (1+3+1)*sizeof(Scalar);
    if (third_law) mem_transfer += n_calc*10*sizeof(Scalar);
    if (m_prof) m_prof->pop(flops, mem_transfer);
    }
void EAMForceCompute::set_neighbor_list(boost::shared_ptr<NeighborList> nlist)
    {
    m_nlist = nlist;
    assert(m_nlist);
    }
Scalar EAMForceCompute::get_r_cut()
    {
    return m_r_cut;
    }
void export_EAMForceCompute()
    {
    scope in_eam = class_<EAMForceCompute, boost::shared_ptr<EAMForceCompute>, bases<ForceCompute>, boost::noncopyable >
        ("EAMForceCompute", init< boost::shared_ptr<SystemDefinition>, char*, int>())

    .def("set_neighbor_list", &EAMForceCompute::set_neighbor_list)
    .def("get_r_cut", &EAMForceCompute::get_r_cut)
    ;
    }

#ifdef WIN32
#pragma warning( pop )
#endif
