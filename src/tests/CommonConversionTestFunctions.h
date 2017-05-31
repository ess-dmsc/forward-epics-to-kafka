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

template <typename FB_type>
void EPICS_To_FB_TimeComparison(const pv::PVTimeStamp timeStamp, const FB_type *fsd_data) {
  pv::TimeStamp timeStampStruct;
  timeStamp.get(timeStampStruct);
  std::uint64_t compTime = (timeStampStruct.getEpicsSecondsPastEpoch() + 631152000L) * 1000000000L + timeStampStruct.getNanoseconds();
  EXPECT_EQ(compTime, fsd_data->timestamp());
}

size_t GetNrOfElements(FSD::FastSamplingData const *fsd_data);

#endif /* CommonConversionTestFunctions_h */
