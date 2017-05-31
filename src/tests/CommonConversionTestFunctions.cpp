//
//  CommonConversionTestFunctions.cpp
//  forward-epics-to-kafka
//
//  Created by Jonas Nilsson on 2017-03-17.
//  Copyright © 2017 European Spallation Source. All rights reserved.
//

#include "CommonConversionTestFunctions.h"
#include <gtest/gtest.h>

size_t GetNrOfElements(FSD::FastSamplingData const *fsd_data) {
    if (fsd_data->data_type() == FSD::type_int8) {
        return static_cast<const FSD::int8*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_uint8) {
        return static_cast<const FSD::uint8*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_uint8) {
        return static_cast<const FSD::uint8*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_int16) {
        return static_cast<const FSD::int16*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_uint16) {
        return static_cast<const FSD::uint16*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_int32) {
        return static_cast<const FSD::int32*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_uint32) {
        return static_cast<const FSD::uint32*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_int64) {
        return static_cast<const FSD::int64*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_uint64) {
        return static_cast<const FSD::uint64*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_float32) {
        return static_cast<const FSD::float32*>(fsd_data->data())->value()->size();
    } else if (fsd_data->data_type() == FSD::type_float64) {
        return static_cast<const FSD::float64*>(fsd_data->data())->value()->size();
    }
    return 0;
}
