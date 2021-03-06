/*
Highly Optimized Object-oriented Many-particle Dynamics -- Blue Edition
(HOOMD-blue) Open Source Software License Copyright 2009-2014 The Regents of
the University of Michigan All rights reserved.

HOOMD-blue may contain modifications ("Contributions") provided, and to which
copyright is held, by various Contributors who have granted The Regents of the
University of Michigan the right to modify and/or distribute such Contributions.

You may redistribute, use, and create derivate works of HOOMD-blue, in source
and binary forms, provided you abide by the following conditions:

* Redistributions of source code must retain the above copyright notice, this
list of conditions, and the following disclaimer both in the code and
prominently in any materials provided with the distribution.

* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions, and the following disclaimer in the documentation and/or
other materials provided with the distribution.

* All publications and presentations based on HOOMD-blue, including any reports
or published results obtained, in whole or in part, with HOOMD-blue, will
acknowledge its use according to the terms posted at the time of submission on:
http://codeblue.umich.edu/hoomd-blue/citations.html

* Any electronic documents citing HOOMD-Blue will link to the HOOMD-Blue website:
http://codeblue.umich.edu/hoomd-blue/

* Apart from the above required attributions, neither the name of the copyright
holder nor the names of HOOMD-blue's contributors may be used to endorse or
promote products derived from this software without specific prior written
permission.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND/OR ANY
WARRANTIES THAT THIS SOFTWARE IS FREE OF INFRINGEMENT ARE DISCLAIMED.

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Maintainer: phillicl

#include <boost/python.hpp>
using namespace boost::python;

#include "TableAngleForceCompute.h"

#include <stdexcept>

/*! \file TableAngleForceCompute.cc
    \brief Defines the TableAngleForceCompute class
*/

#undef VT0
#undef VT1

using namespace std;

// SMALL a relatively small number
#define SMALL 0.001f

/*! \param sysdef System to compute forces on
    \param table_width Width the tables will be in memory
    \param log_suffix Name given to this instance of the table potential
*/
TableAngleForceCompute::TableAngleForceCompute(boost::shared_ptr<SystemDefinition> sysdef,
                               unsigned int table_width,
                               const std::string& log_suffix)
        : ForceCompute(sysdef), m_table_width(table_width)
    {
    m_exec_conf->msg->notice(5) << "Constructing TableAngleForceCompute" << endl;

    assert(m_pdata);

    // access the angle data for later use
    m_angle_data = m_sysdef->getAngleData();

    // check for some silly errors a user could make
    if (m_angle_data->getNTypes() == 0)
        {
        m_exec_conf->msg->error() << "angle.table: No angle types specified" << endl;
        throw runtime_error("Error initializing TableAngleForceCompute");
        }


    if (table_width == 0)
        {
        m_exec_conf->msg->error() << "angle.table: Table width of 0 is invalid" << endl;
        throw runtime_error("Error initializing TableAngleForceCompute");
        }




    // allocate storage for the tables and parameters
    GPUArray<Scalar2> tables(m_table_width, m_angle_data->getNTypes(), exec_conf);
    m_tables.swap(tables);
    assert(!m_tables.isNull());

    // helper to compute indices
    Index2D table_value(m_tables.getPitch(),m_angle_data->getNTypes());
    m_table_value = table_value;

    m_log_name = std::string("angle_table_energy") + log_suffix;
    }

TableAngleForceCompute::~TableAngleForceCompute()
        {
        m_exec_conf->msg->notice(5) << "Destroying TableAngleForceCompute" << endl;
        }

/*! \param type Type of the angle to set parameters for
    \param V Table for the potential V
    \param T Table for the torque T (must be - dV / dtheta)
    \post Values from \a V and \a T are copied into the interal storage for type pair (type)
*/
void TableAngleForceCompute::setTable(unsigned int type,
                              const std::vector<Scalar> &V,
                              const std::vector<Scalar> &T)
    {

    // make sure the type is valid
    if (type >= m_angle_data->getNTypes())
        {
        m_exec_conf->msg->error() << "angle.table: Invalid angle type specified" << endl << endl;
        throw runtime_error("Error setting parameters in TableAngleForceCompute");
        }


    // access the arrays
    ArrayHandle<Scalar2> h_tables(m_tables, access_location::host, access_mode::readwrite);


    if (V.size() != m_table_width || T.size() != m_table_width)
        {
        m_exec_conf->msg->error() << "angle.table: table provided to setTable is not of the correct size" << endl;
        throw runtime_error("Error initializing TableAngleForceCompute");
        }


    // fill out the table
    for (unsigned int i = 0; i < m_table_width; i++)
        {
        h_tables.data[m_table_value(i, type)].x = V[i];
        h_tables.data[m_table_value(i, type)].y = T[i];
        }
    }

/*! TableAngleForceCompute provides
    - \c angle_table_energy
*/
std::vector< std::string > TableAngleForceCompute::getProvidedLogQuantities()
    {
    vector<string> list;
    list.push_back(m_log_name);
    return list;
    }

Scalar TableAngleForceCompute::getLogValue(const std::string& quantity, unsigned int timestep)
    {
    if (quantity == m_log_name)
        {
        compute(timestep);
        return calcEnergySum();
        }
    else
        {
        m_exec_conf->msg->error() << "angle.table: " << quantity << " is not a valid log quantity for TableAngleForceCompute" << endl;
        throw runtime_error("Error getting log value");
        }
    }

/*! \post The table based forces are computed for the given timestep.
\param timestep specifies the current time step of the simulation
*/
void TableAngleForceCompute::computeForces(unsigned int timestep)
    {

    // start the profile for this compute
    if (m_prof) m_prof->push("Table Angle");


    // access the particle data
    ArrayHandle<Scalar4> h_pos(m_pdata->getPositions(), access_location::host, access_mode::read);
    ArrayHandle<Scalar4> h_force(m_force,access_location::host, access_mode::overwrite);
    ArrayHandle<Scalar> h_virial(m_virial,access_location::host, access_mode::overwrite);
    ArrayHandle<unsigned int> h_rtag(m_pdata->getRTags(), access_location::host, access_mode::read);

    // there are enough other checks on the input data: but it doesn't hurt to be safe
    assert(h_force.data);
    assert(h_virial.data);
    assert(h_pos.data);
    assert(h_rtag.data);

    unsigned int virial_pitch = m_virial.getPitch();

    // Zero data for force calculation.
    memset((void*)h_force.data,0,sizeof(Scalar4)*m_force.getNumElements());
    memset((void*)h_virial.data,0,sizeof(Scalar)*m_virial.getNumElements());

    // get a local copy of the simulation box too
    const BoxDim& box = m_pdata->getBox();

    // access the table data
    ArrayHandle<Scalar2> h_tables(m_tables, access_location::host, access_mode::read);

    // for each of the angles
    const unsigned int size = (unsigned int)m_angle_data->getN();
    for (unsigned int i = 0; i < size; i++)
        {
        // lookup the tag of each of the particles participating in the angle
        const AngleData::members_t& angle = m_angle_data->getMembersByIndex(i);
        assert(angle.tag[0] < m_pdata->getNGlobal());
        assert(angle.tag[1] < m_pdata->getNGlobal());
        assert(angle.tag[2] < m_pdata->getNGlobal());

        // transform a, b, and c into indicies into the particle data arrays
        // MEM TRANSFER: 6 ints
        unsigned int idx_a = h_rtag.data[angle.tag[0]];
        unsigned int idx_b = h_rtag.data[angle.tag[1]];
        unsigned int idx_c = h_rtag.data[angle.tag[2]];

        // throw an error if this angle is incomplete
        if (idx_a == NOT_LOCAL|| idx_b == NOT_LOCAL || idx_c == NOT_LOCAL)
            {
            this->m_exec_conf->msg->error() << "angle.table: angle " <<
                angle.tag[0] << " " << angle.tag[1] << " " << angle.tag[2] << " incomplete." << endl << endl;
            throw std::runtime_error("Error in angle calculation");
            }

        assert(idx_a < m_pdata->getN()+m_pdata->getNGhosts());
        assert(idx_b < m_pdata->getN()+m_pdata->getNGhosts());
        assert(idx_c < m_pdata->getN()+m_pdata->getNGhosts());

        // calculate d\vec{r}
        Scalar3 dab;
        dab.x = h_pos.data[idx_a].x - h_pos.data[idx_b].x;
        dab.y = h_pos.data[idx_a].y - h_pos.data[idx_b].y;
        dab.z = h_pos.data[idx_a].z - h_pos.data[idx_b].z;

        Scalar3 dcb;
        dcb.x = h_pos.data[idx_c].x - h_pos.data[idx_b].x;
        dcb.y = h_pos.data[idx_c].y - h_pos.data[idx_b].y;
        dcb.z = h_pos.data[idx_c].z - h_pos.data[idx_b].z;

        Scalar3 dac;
        dac.x = h_pos.data[idx_a].x - h_pos.data[idx_c].x; // used for the 1-3 JL interaction
        dac.y = h_pos.data[idx_a].y - h_pos.data[idx_c].y;
        dac.z = h_pos.data[idx_a].z - h_pos.data[idx_c].z;


        // apply minimum image conventions to all 3 vectors
        dab = box.minImage(dab);
        dcb = box.minImage(dcb);
        dac = box.minImage(dac);

        Scalar delta_th = Scalar(M_PI)/Scalar(m_table_width - 1);

        // start computing the force
        Scalar rsqab = dab.x*dab.x+dab.y*dab.y+dab.z*dab.z;
        Scalar rab = sqrt(rsqab);
        Scalar rsqcb = dcb.x*dcb.x+dcb.y*dcb.y+dcb.z*dcb.z;
        Scalar rcb = sqrt(rsqcb);

        // cosine of theta
        Scalar c_abbc = dab.x*dcb.x+dab.y*dcb.y+dab.z*dcb.z;
        c_abbc /= rab*rcb;

        if (c_abbc > 1.0) c_abbc = 1.0;
        if (c_abbc < -1.0) c_abbc = -1.0;

        //1/sine of theta
        Scalar s_abbc = sqrt(1.0 - c_abbc*c_abbc);
        if (s_abbc < SMALL) s_abbc = SMALL;
        s_abbc = 1.0/s_abbc;

        //theta
        Scalar theta = acos(c_abbc);

        // precomputed term
        Scalar value_f = theta / delta_th;

        // compute index into the table and read in values

        /// Here we use the table!!
        unsigned int angle_type = m_angle_data->getTypeByIndex(i);
        unsigned int value_i = floor(value_f);
        Scalar2 VT0 = h_tables.data[m_table_value(value_i, angle_type)];
        Scalar2 VT1 = h_tables.data[m_table_value(value_i+1, angle_type)];
        // unpack the data
        Scalar V0 = VT0.x;
        Scalar V1 = VT1.x;
        Scalar T0 = VT0.y;
        Scalar T1 = VT1.y;

        // compute the linear interpolation coefficient
        Scalar f = value_f - Scalar(value_i);

        // interpolate to get V and T;
        Scalar V = V0 + f * (V1 - V0);
        Scalar T = T0 + f * (T1 - T0);

        Scalar a =  T*s_abbc;
        Scalar a11 = a*c_abbc/rsqab;
        Scalar a12 = -a / (rab*rcb);
        Scalar a22 = a*c_abbc / rsqcb;


        Scalar fab[3], fcb[3];

        fab[0] = a11*dab.x + a12*dcb.x;
        fab[1] = a11*dab.y + a12*dcb.y;
        fab[2] = a11*dab.z + a12*dcb.z;

        fcb[0] = a22*dcb.x + a12*dab.x;
        fcb[1] = a22*dcb.y + a12*dab.y;
        fcb[2] = a22*dcb.z + a12*dab.z;

        Scalar angle_eng = V*Scalar(1.0/3.0);

        // compute 1/3 of the virial, 1/3 for each atom in the angle
        // symmetrized version of virial tensor
        Scalar angle_virial[6];
        angle_virial[0] = Scalar(1./3.) * ( dab.x*fab[0] + dcb.x*fcb[0] );
        angle_virial[1] = Scalar(1./3.) * ( dab.y*fab[0] + dcb.y*fcb[0] );
        angle_virial[2] = Scalar(1./3.) * ( dab.z*fab[0] + dcb.z*fcb[0] );
        angle_virial[3] = Scalar(1./3.) * ( dab.y*fab[1] + dcb.y*fcb[1] );
        angle_virial[4] = Scalar(1./3.) * ( dab.z*fab[1] + dcb.z*fcb[1] );
        angle_virial[5] = Scalar(1./3.) * ( dab.z*fab[2] + dcb.z*fcb[2] );

        // Now, apply the force to each individual atom a,b,c, and accumlate the energy/virial
        // only apply force to local atoms
        if (idx_a < m_pdata->getN())
            {
            h_force.data[idx_a].x += fab[0];
            h_force.data[idx_a].y += fab[1];
            h_force.data[idx_a].z += fab[2];
            h_force.data[idx_a].w += angle_eng;
            for (int j = 0; j < 6; j++)
                h_virial.data[j*virial_pitch+idx_a]  += angle_virial[j];
            }

        if (idx_b < m_pdata->getN())
            {
            h_force.data[idx_b].x -= fab[0] + fcb[0];
            h_force.data[idx_b].y -= fab[1] + fcb[1];
            h_force.data[idx_b].z -= fab[2] + fcb[2];
            h_force.data[idx_b].w += angle_eng;
            for (int j = 0; j < 6; j++)
                h_virial.data[j*virial_pitch+idx_b]  += angle_virial[j];
            }

        if (idx_c < m_pdata->getN())
            {
            h_force.data[idx_c].x += fcb[0];
            h_force.data[idx_c].y += fcb[1];
            h_force.data[idx_c].z += fcb[2];
            h_force.data[idx_c].w += angle_eng;
            for (int j = 0; j < 6; j++)
                h_virial.data[j*virial_pitch+idx_c]  += angle_virial[j];
            }
        }

    if (m_prof) m_prof->pop();
    }

//! Exports the TableAngleForceCompute class to python
void export_TableAngleForceCompute()
    {
    class_<TableAngleForceCompute, boost::shared_ptr<TableAngleForceCompute>, bases<ForceCompute>, boost::noncopyable >
    ("TableAngleForceCompute", init< boost::shared_ptr<SystemDefinition>, unsigned int, const std::string& >())
    .def("setTable", &TableAngleForceCompute::setTable)
    ;
    }
