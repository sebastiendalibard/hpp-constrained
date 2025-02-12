// Copyright (C) 2011 by Sebastien Dalibard.
//
// This file is part of the hpp-constrained.
//
// hpp-constrained is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// hpp-constrained is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with hpp-constrained.  If not, see <http://www.gnu.org/licenses/>.

#include <limits>

#include <hpp/constrained/config-extendor.hh>

namespace hpp {
  namespace constrained {

    ConfigExtendor::ConfigExtendor(hpp::model::DeviceShPtr robot):
      ConfigProjector(robot)
    {
      vectorN config = robot->currentConfiguration();
      vectorN mask (robot->numberDof(),1);
      configConstraint_ = new ChppGikConfigurationConstraint(*robot, config, mask);
    }

    ConfigExtendor::~ConfigExtendor()
    {
    }

    CkwsConfigShPtr
    ConfigExtendor::extendOneStep(const CkwsConfig & extendTo,
				  const CkwsConfig & extendFrom)
    {
      /* Putting dynamic robot in extendFrom configuration */
      std::vector<double> dofs;
      extendFrom.getDofValues(dofs);
      vectorN jrlCfg(dofs.size());
      robot_->kwsToJrlDynamicsDofValues(dofs,jrlCfg);
      robot_->currentConfiguration(jrlCfg);
      robot_->computeForwardKinematics();

      return (extendOneStep(extendTo));
    }

    CkwsConfigShPtr
    ConfigExtendor::extendOneStep(const CkwsConfig & extendTo)
    {
      CkwsConfigShPtr resCfg;

      vectorN currentConfig(robot_->numberDof());
      currentConfig = robot_->currentConfiguration();

      std::vector<double> kwsdofs;
      extendTo.getDofValues(kwsdofs);
      vectorN jrlextendTo(kwsdofs.size());
      robot_->kwsToJrlDynamicsDofValues(kwsdofs,jrlextendTo);

      configConstraint_->target(jrlextendTo);
      configConstraint_->computeValue();
      double initialValue = norm_2(configConstraint_->value());

      addConstraint(configConstraint_);

      if(!optimizeOneStep()) {
	return resCfg;
      }
      removeLastConstraint();

      currentConfig = robot_->currentConfiguration();
      if ( project(currentConfig) != KD_OK) {
	return resCfg;
      }
      configConstraint_->computeValue();
      double newValue  = norm_2(configConstraint_->value());

      if (newValue > initialValue - progressThreshold_) {
	return resCfg;
      }

      if (!areConstraintsSatisfied()) {
	return resCfg;
      }
 
      robot_->jrlDynamicsToKwsDofValues(currentConfig,kwsdofs);

      resCfg = CkwsConfig::create(robot_,kwsdofs);
      cache_.insert(*resCfg);

      return resCfg;
    }
  } //end of namespace constrained
} //end of namespace hpp
