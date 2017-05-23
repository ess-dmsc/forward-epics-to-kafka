//
//  CommonConversionTestFunctions.h
//  forward-epics-to-kafka
//
//  Created by Jonas Nilsson on 2017-03-17.
//  Copyright © 2017 European Spallation Source. All rights reserved.
//

#ifndef CommonConversionTestFunctions_h
#define CommonConversionTestFunctions_h

#include <pv/pvData.h>
#include <pv/pvTimeStamp.h>
#include "schemas/fsdc/fsdc_FastSamplingData_generated.h"

namespace pv = epics::pvData;

void EPICS_To_FB_TimeComparison(pv::PVTimeStamp const timeStamp, FSD::FastSamplingData const *fsd_data);

size_t GetNrOfElements(FSD::FastSamplingData const *fsd_data);

#endif /* CommonConversionTestFunctions_h */
