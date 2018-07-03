import flatbuffers
import time
import math
from .f142_logdata import LogData, Value, Int, Double


def check_double_value_and_equality(log_data, expected_value):
    """
    Initialises the log data object from bytes and checks the union table
    and converts to Python Double then compares against the expected Double value.

    :param log_data: Log data object from the received stream buffer
    :param expected_value: Double value to compare against
    :return: none
    """
    union_double = Double.Double()
    union_double.Init(log_data.Value().Bytes, log_data.Value().Pos)
    union_value = union_double.Value()
    assert math.isclose(expected_value, union_value)


def check_message_pv_name_and_value_type(log_data, value_type, pv_name):
    """
    Checks the message name (PV) and value type (type of PV).

    :param log_data: Log data object from the received stream buffer
    :param value_type: Flatbuffers value type
    :param pv_name: Byte encoded string of the PV/channel name
    :return: none
    """
    assert value_type == log_data.ValueType()
    assert pv_name == log_data.SourceName()


def create_flatbuffers_object(file_identifier):
    """
    Create a sample flatbuffers buffer.

    :param file_identifier: The flatbuffers schema ID
    :return: The constructed buffer
    """
    builder = flatbuffers.Builder(512)
    source_name = builder.CreateString("test")
    Int.IntStart(builder)
    Int.IntAddValue(builder, 2)
    int1 = Int.IntEnd(builder)
    LogData.LogDataStart(builder)
    LogData.LogDataAddSourceName(builder, source_name)
    LogData.LogDataAddValueType(builder, Value.Value().Int)
    LogData.LogDataAddValue(builder, int1)
    LogData.LogDataAddTimestamp(builder, int(time.time()))
    end_offset = LogData.LogDataEnd(builder)
    builder.Finish(end_offset)
    buf = builder.Output()
    buf[4:8] = bytes(file_identifier, encoding="utf-8")
    return buf
