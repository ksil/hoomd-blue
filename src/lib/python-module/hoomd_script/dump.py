# Highly Optimized Object-Oriented Molecular Dynamics (HOOMD) Open
# Source Software License
# Copyright (c) 2008 Ames Laboratory Iowa State University
# All rights reserved.

# Redistribution and use of HOOMD, in source and binary forms, with or
# without modification, are permitted, provided that the following
# conditions are met:

# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.

# * Neither the name of the copyright holder nor the names HOOMD's
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.

# Disclaimer

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND
# CONTRIBUTORS ``AS IS''  AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 

# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# $Id$
# $URL$

## \package hoomd_script.dump
# \brief Commands that dump particles to files
#
# Details

import hoomd;
import globals;
import analyze;

## Writes simulation snapshots in the hoomd_xml format
#
# Every \a period time steps, a new file will be created. The state of the 
# particles at that timestep is written to the file in the hoomd_xml format.
class xml(analyze._analyzer):
	## Initialize the hoomd_xml writer
	#
	# \param self Python-required class instance variable
	# \param filename Base of the time name
	# \param period Number of time steps between file dumps
	# 
	# \b Examples:<br>
	# dump.xml(filename="atoms.dump", period=1000)<br>
	# xml = dump.xml(filename="particles", period=1e5)<br>
	#
	# By default, only particle positions are output to the dump files. This can be changed
	# with set_params().
	def __init__(self, filename, period):
		print "dump.xml(filename =", filename, ", period=", period, ")";
	
		# initialize base class
		analyze._analyzer.__init__(self);
		
		# create the c++ mirror class
		self.cpp_analyzer = hoomd.HOOMDDumpWriter(globals.particle_data, filename);
		globals.system.addAnalyzer(self.cpp_analyzer, self.analyzer_name, period);

	## Change xml write parameters
	#
	# \param self Python-required class instance variable
	# \param position (if set) Set to True/False to enable/disable the output of particle positions in the xml file
	# \param velocity (if set) Set to True/False to enable/disable the output of particle velocities in the xml file
	# \param type (if set) Set to True/False to enable/disable the output of particle types in the xml file
	# 
	# \b Examples:<br>
	# xml.set_params(type=False)<br>
	# xml.set_params(position=False, type=False, velocity=True)<br>
	# xml.set_params(type=True, position=True)<br>
	def set_params(self, position=None, velocity=None, type=None):
		print "xml.set_params(position=", position, ", velocity=",velocity,", type=", type, ")";
	
		# check that proper initialization has occured
		if self.cpp_analyzer == None:
			print "Bug in hoomd_script: cpp_analyzer not set, please report";
			raise RuntimeError('Error setting xml parameters');
			
		if position != None:
			self.cpp_analyzer.outputPosition(position);

		if velocity != None:
			self.cpp_analyzer.outputVelocity(velocity);
			
		if type != None:
			self.cpp_analyzer.outputType(type);
			
## Writes a simulation snapshot in the mol2 format
#
# At the first time step run() after initializing the dump, the state of the 
# particles at that timestep is written to the file in the .mol2 file format.
# The intended usage is to generate a single structure file that can be used by
# VMD for reading in particle names and bond topology Use in conjunction with
# dump.dcd for reading the full simulation trajectory into VMD.
class mol2(analyze._analyzer):
	## Initialize the mol2 writer
	#
	# \param self Python-required class instance variable
	# \param filename File name to write to
	# 
	# \b Examples:<br>
	# dump.mol2(filename="structure.mol2")<br>
	def __init__(self, filename):
		print "dump.mol2(filename =", filename, ")";
	
		# initialize base class
		analyze._analyzer.__init__(self);
		
		# create the c++ mirror class
		self.cpp_analyzer = hoomd.MOL2DumpWriter(globals.particle_data, filename);
		# run it with a ludicrous period so that it is really only run once
		globals.system.addAnalyzer(self.cpp_analyzer, self.analyzer_name, int(1e9));
	
## Writes simulation snapshots in the DCD format
#
# Every \a period time steps a new simulation snapshot is written to the 
# specified file in the DCD file format. DCD only stores particle positions
# but is decently space efficient and extremely fast to read and write. VMD
# can load 100's of MiB of trajectory data in mere seconds.
#
# Use in conjunction with dump.mol2 so that VMD has information on the
# particle names and bond topology.
#
# Due to constraints of the DCD file format, once you stop writing to
# a file via disable(), you cannot continue writing to the same file,
# nor can you change the period of the dump at any time. Either of these tasks 
# can be performed by creating a new dump file with the needed settings.
class dcd(analyze._analyzer):
	## Initialize the dcd writer
	#
	# \param self Python-required class instance variable
	# \param filename File name to write to
	# \param period Number of time steps between file dumps
	# 
	# \b Examples:<br>
	# dump.dcd(filename="trajectory.dcd", period=1000)<br>
	# dcd = dump.dcd(filename"data/dump.dcd", period=1000)
	def __init__(self, filename, period):
		print "dump.dcd(filename =", filename, ", period=", period, ")";
		
		# initialize base class
		analyze._analyzer.__init__(self);
		
		# create the c++ mirror class
		self.cpp_analyzer = hoomd.DCDDumpWriter(globals.particle_data, filename, int(period));
		globals.system.addAnalyzer(self.cpp_analyzer, self.analyzer_name, int(period));
	
	def enable(self):
		print "updater.enable()";
		
		if self.enabled == False:
			print "Error: you cannot re-enable DCD output after it has been disabled";
			raise RuntimeError('Error enabling updater');
	
	def set_period(self, period):
		print "updater.set_period(", period, ")";
		
		print "Error: you cannot change the period of a dcd dump writer";
		raise RuntimeError('Error changing updater period');