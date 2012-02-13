# -- start license --
# Highly Optimized Object-oriented Many-particle Dynamics -- Blue Edition
# (HOOMD-blue) Open Source Software License Copyright 2008-2011 Ames Laboratory
# Iowa State University and The Regents of the University of Michigan All rights
# reserved.

# HOOMD-blue may contain modifications ("Contributions") provided, and to which
# copyright is held, by various Contributors who have granted The Regents of the
# University of Michigan the right to modify and/or distribute such Contributions.

# You may redistribute, use, and create derivate works of HOOMD-blue, in source
# and binary forms, provided you abide by the following conditions:

# * Redistributions of source code must retain the above copyright notice, this
# list of conditions, and the following disclaimer both in the code and
# prominently in any materials provided with the distribution.

# * Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions, and the following disclaimer in the documentation and/or
# other materials provided with the distribution.

# * All publications and presentations based on HOOMD-blue, including any reports
# or published results obtained, in whole or in part, with HOOMD-blue, will
# acknowledge its use according to the terms posted at the time of submission on:
# http://codeblue.umich.edu/hoomd-blue/citations.html

# * Any electronic documents citing HOOMD-Blue will link to the HOOMD-Blue website:
# http://codeblue.umich.edu/hoomd-blue/

# * Apart from the above required attributions, neither the name of the copyright
# holder nor the names of HOOMD-blue's contributors may be used to endorse or
# promote products derived from this software without specific prior written
# permission.

# Disclaimer

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND/OR ANY
# WARRANTIES THAT THIS SOFTWARE IS FREE OF INFRINGEMENT ARE DISCLAIMED.

# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# -- end license --

# Maintainer: joaander / All Developers are free to add commands for new features

## \package hoomd_script.external
# \brief Commands that create external forces on particles
#
# Apply an %external force to all particles in the simulation. This module organizes all external forces.
# As an example, a force derived from a %periodic potential can be used to induce a concentration modulation
# in the system.

import globals;
import force;
import hoomd;
import util;
import init;
import data;
import tune;

import sys;

# \brief Defines external potential coefficients
# The coefficients for all %external force are specified using this class. Coefficients are specified per-particle
# type.
#
# There are two ways to set the coefficients for a particular %external %force.
# The first way is to save the %external %force in a variable and call set() directly.
# To see an example of this, see the documentation for the package.
#
# The second method is to build the coeff class first and then assign it to the
# %external %force. There are some advantages to this method in that you could specify a
# complicated set of %external %force coefficients in a separate python file and import it into
# your job script.
#
# Example:
# \code
# my_coeffs = external.coeff();
# my_external_force.force_coeff.set('A', A=1.0, i=1, w=0.02, p=3)
# my_external_force.force_coeff.set('B', A=-1.0, i=1, w=0.02, p=3)
# \endcode
class coeff:

    ## \internal
    # \brief Initializes the class
    # \details
    # The main task to be performed during initialization is just to init some variables
    # \param self Python required class instance variable
    def __init__(self):
        self.values = {};
        self.default_coeff = {}

    ## \var values
    # \internal
    # \brief Contains the vector of set values in a dictionary

    ## \var default_coeff
    # \internal
    # \brief default_coeff['coeff'] lists the default value for \a coeff, if it is set

    ## \internal
    # \brief Sets a default value for a given coefficient
    # \details
    # \param name Name of the coefficient to for which to set the default
    # \param value Default value to set
    #
    # Some coefficients have reasonable default values and the user should not be burdened with typing them in
    # all the time. set_default_coeff() sets
    def set_default_coeff(self, name, value):
        self.default_coeff[name] = value;

    ## Sets parameters for one particle type
    # \param type Type of particle
    # \param coeff Named coefficients (see below for examples)
    #
    # Calling set() results in one or more parameters being set for a particle type. Types are identified
    # by name, and parameters are also added by name. Which parameters you need to specify depends on the %external
    # %force you are setting these coefficients for, see the corresponding documentation.
    #
    # All possible particle types as defined in the simulation box must be specified before executing run().
    # You will receive an error if you fail to do so. It is not an error, however, to specify coefficients for
    # particle types that do not exist in the simulation. This can be useful in defining a %force field for many
    # different types of particles even when some simulations only include a subset.
    #
    # To set the same coefficients between many particle types, provide a list of type names instead of a single
    # one. All types in the list will be set to the same parameters. A convenient wildcard that lists all types
    # of particles in the simulation can be gotten from a saved \c system from the init command.
    #
    # \b Examples:
    # \code
    # coeff.set('A', A=1.0, i=1, w=0.02, p=3)
    # coeff.set('B', A=-1.0, i=1, w=0.02, p=3)
    # coeff.set(['A','B'], i=1, w=0.02, p=3)
    # \endcode
    #
    # \note Single parameters can be updated. If both epsilon and sigma have already been set for a particle type,
    # then executing coeff.set('A', A =1.0) will %update the value of A and leave the other parameters as they
    # were previously set.
    #
    def set(self, type, **coeffs):
        util.print_status_line();

        # listify the input
        if isinstance(type, str):
            type = [type];

        for typei in type:
            self.set_single(typei, coeffs);

    ## \internal
    # \brief Sets a single parameter
    def set_single(self, type, coeffs):
        # create the type identifier if it hasn't been created yet
        if (not type in self.values):
            self.values[type] = {};

        # update each of the values provided
        if len(coeffs) == 0:
            print >> sys.stderr, "\n***Error! No coefficents specified\n";
        for name, val in coeffs.items():
            self.values[type][name] = val;

        # set the default values
        for name, val in self.default_coeff.items():
            # don't override a coeff if it is already set
            if not name in self.values[type]:
                self.values[type][name] = val;

    ## \internal
    # \brief Verifies that all values are set
    # \details
    # \param self Python required self variable
    # \param required_coeffs list of required variables
    #
    # This can only be run after the system has been initialized
    def verify(self, required_coeffs):
        # first, check that the system has been initialized
        if not init.is_initialized():
            print >> sys.stderr, "\n***Error! Cannot verify force coefficients before initialization\n";
            raise RuntimeError('Error verifying force coefficients');

        # get a list of types from the particle data
        ntypes = globals.system_definition.getParticleData().getNTypes();
        type_list = [];
        for i in xrange(0,ntypes):
            type_list.append(globals.system_definition.getParticleData().getNameByType(i));

        valid = True;
        # loop over all possible types and verify that all required variables are set
        for i in xrange(0,ntypes):
            type = type_list[i];

            # verify that all required values are set by counting the matches
            count = 0;
            for coeff_name in self.values[type].keys():
                if not coeff_name in required_coeffs:
                    print "Notice: Possible typo? Force coeff", coeff_name, "is specified for type", type, \
                          ", but is not used by the external force";
                else:
                    count += 1;

            if count != len(required_coeffs):
                print >> sys.stderr, "\n***Error! Particle type", type, "is missing required coefficients\n";
                valid = False;

        return valid;

    ## \internal
    # \brief Gets the value of a single %external %force coefficient
    # \detail
    # \param type Name of particle type
    # \param coeff_name Coefficient to get
    def get(self, type, coeff_name):
        if type not in self.values:
            print >> sys.stderr, "\nBug detected in external.coeff. Please report\n"
            raise RuntimeError("Error setting external coeff");

        return self.values[type][coeff_name];

## \internal
# \brief Base class for external forces
#
# An external_force in hoomd_script reflects a PotentialExternal in c++. It is responsible
# for all high-level management that happens behind the scenes for hoomd_script
# writers. 1) The instance of the c++ external force itself is tracked and added to the
# System 2) methods are provided for disabling the force from being added to the
# net force on each particle
class _external_force(force._force):
    ## \internal
    # \brief Constructs the external force
    #
    # \param name name of the external force instance
    #
    # Initializes the cpp_force to None.
    # If specified, assigns a name to the instance
    # Assigns a name to the force in force_name;
    def __init__(self, name=None):
        # initialize the base class
        force._force.__init__(self, name);

        self.cpp_force = None;

        # setup the coefficient vector
        self.force_coeff = coeff();

        # increment the id counter
        id = _external_force.cur_id;
        _external_force.cur_id += 1;

        self.force_name = "external_force%d" % (id);
        self.enabled = True;
        globals.external_forces.append(self);

        # create force data iterator
        self.external_forces = data.force_data(self);

    def update_coeffs(self):
        coeff_list = self.required_coeffs;
        # check that the force coefficients are valid
        if not self.force_coeff.verify(coeff_list):
           print >> sys.stderr, "\n***Error: Not all force coefficients are set\n";
           raise RuntimeError("Error updating force coefficients");

        # set all the params
        ntypes = globals.system_definition.getParticleData().getNTypes();
        type_list = [];
        for i in xrange(0,ntypes):
            type_list.append(globals.system_definition.getParticleData().getNameByType(i));

        for i in xrange(0,ntypes):
            # build a dict of the coeffs to pass to proces_coeff
            coeff_dict = {};
            for name in coeff_list:
                coeff_dict[name] = self.force_coeff.get(type_list[i], name);

            param = self.process_coeff(coeff_dict);
            self.cpp_force.setParams(i, param);

    ## \var enabled
    # \internal
    # \brief True if the force is enabled

    ## \var cpp_force
    # \internal
    # \brief Stores the C++ side ForceCompute managed by this class

    ## \var force_name
    # \internal
    # \brief The Force's name as it is assigned to the System

    ## \internal
    # \brief Checks that proper initialization has completed
    def check_initialization(self):
        # check that we have been initialized properly
        if self.cpp_force is None:
            print >> sys.stderr, "\nBug in hoomd_script: cpp_force not set, please report\n";
            raise RuntimeError();


    ## Disables the force
    #
    # \b Examples:
    # \code
    # force.disable()
    # \endcode
    #
    # Executing the disable command will remove the force from the simulation.
    # Any run() command executed after disabling a force will not calculate or
    # use the force during the simulation. A disabled force can be re-enabled
    # with enable()
    #
    # To use this command, you must have saved the force in a variable, as
    # shown in this example:
    # \code
    # force = external.some_force()
    # # ... later in the script
    # force.disable()
    # \endcode
    def disable(self):
        util.print_status_line();
        self.check_initialization();

        # check if we are already disabled
        if not self.enabled:
            print "***Warning! Ignoring command to disable a force that is already disabled";
            return;

        self.enabled = False;

        # remove the compute from the system
        globals.system.removeCompute(self.force_name);

    ## Benchmarks the force computation
    # \param n Number of iterations to average the benchmark over
    #
    # \b Examples:
    # \code
    # t = force.benchmark(n = 100)
    # \endcode
    #
    # The value returned by benchmark() is the average time to perform the force
    # computation, in milliseconds. The benchmark is performed by taking the current
    # positions of all particles in the simulation and repeatedly calculating the forces
    # on them. Thus, you can benchmark different situations as you need to by simply
    # running a simulation to achieve the desired state before running benchmark().
    #
    # \note
    # There is, however, one subtle side effect. If the benchmark() command is run
    # directly after the particle data is initialized with an init command, then the
    # results of the benchmark will not be typical of the time needed during the actual
    # simulation. Particles are not reordered to improve cache performance until at least
    # one time step is performed. Executing run(1) before the benchmark will solve this problem.
    #
    # To use this command, you must have saved the force in a variable, as
    # shown in this example:
    # \code
    # force = external.some_force()
    # # ... later in the script
    # t = force.benchmark(n = 100)
    # \endcode
    def benchmark(self, n):
        self.check_initialization();

        # run the benchmark
        return self.cpp_force.benchmark(int(n))

    ## Enables the force
    #
    # \b Examples:
    # \code
    # force.enable()
    # \endcode
    #
    # See disable() for a detailed description.
    def enable(self):
        util.print_status_line();
        self.check_initialization();

        # check if we are already disabled
        if self.enabled:
            print "***Warning! Ignoring command to enable a force that is already enabled";
            return;

        # add the compute back to the system
        globals.system.addCompute(self.cpp_force, self.force_name);

        self.enabled = True;

# set default counter
_external_force.cur_id = 0;


## Constrain particles to the surface of a sphere
#
# The command %periodic specifies that an external %force should be
# added to every particle in the simulation to induce a periodic modulation
# in the particle concentration at a given point \f$ \vec{r} \f$ (e.g. to
# impose an ordered structure in a diblock copolymer melt)
#
# The external potential \f$ V(\vec{r}) \f$ is implemented using the following formula:
#
# \f[ V(\vec{r}) = A * \tanh\left[\frac{1}{2 \pi p w} \cos\left(\frac{2 \pi p r_i}{L_i}\right)\right] \f]
#
# where \f$ A \f$ is the ordering parameter, \f$p\f$ the periodicity and \f$w\f$ the interface width
# (relative to the box length \f$ L_i \f$ in the \f$ i \f$ -direction). The modulation is one-dimensional,
# i.e. it extends along the cartesian coordinate \f$ i \f$ .
class periodic(_external_force):
    ## Apply a force derived from a %periodic potential to all particles
    #
    # \b Examples:
    # \code
    # periodic = external.periodic()
    # periodic.coeff.set('A', A=1.0, i=1, w=0.02, p=3)
    # periodic.coeff.set('B', A=-1.0, i=1, w=0.02, p=3)
    # \endcode

    def __init__(self):
        util.print_status_line();

        # initialize the base class
        _external_force.__init__(self);

        # create the c++ mirror class
        if not globals.exec_conf.isCUDAEnabled():
            self.cpp_force = hoomd.PotentialExternalLamellar(globals.system_definition);
        else:
            self.cpp_force = hoomd.PotentialExternalLamellarGPU(globals.system_definition);
            self.cpp_force.setBlockSize(tune._get_optimal_block_size('external.periodic'));

        globals.system.addCompute(self.cpp_force, self.force_name);

        # setup the coefficient options
        self.required_coeffs = ['A','i','w','p'];

    def process_coeff(self, coeff):
        A = coeff['A'];
        i = coeff['i'];
        w = coeff['w'];
        p = coeff['p'];

        return hoomd.make_scalar4(hoomd.int_as_scalar(i), A, w, hoomd.int_as_scalar(p));