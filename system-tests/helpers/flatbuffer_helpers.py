from helpers.f142_logdata import Int, Double, String, Long, Value, ArrayFloat
from cmath import isclose

ValueTypes = {
    Value.Value.Int: Int.Int,
    Value.Value.Double: Double.Double,
    Value.Value.String: String.String,
    Value.Value.ArrayFloat: ArrayFloat.ArrayFloat,
}


def check_expected_values(log_data, value_type, pv_name, expected_value=None):
    """
    Checks the message name (PV) and value type (type of PV), and, optionally, the value.

    :param log_data: Log data object from the received stream buffer
    :param value_type: Flatbuffers value type
    :param pv_name: Byte encoded string of the PV/channel name
    :param expected_value: The expected PV value from the flatbuffers message
    :return: None
    """
    assert value_type == log_data.ValueType()
    assert bytes(pv_name, encoding='utf-8') == log_data.SourceName()
    assert log_data.Timestamp() > 0

    if expected_value is not None:
        union_val = ValueTypes[value_type]()
        union_val.Init(log_data.Value().Bytes, log_data.Value().Pos)
        print(f'expected value: {expected_value}, value from message: {union_val.Value()}', flush=True)
        if isinstance(expected_value, float):
            assert isclose(expected_value, union_val.Value())
        else:
            assert expected_value == union_val.Value()


def check_expected_connection_status_values(status_data, expected_value):
    """
    Checks the message name (PV) and value type (type of PV), and, optionally, the value.

    :param status_data: Status data object from the received stream buffer
    :param expected_value: The expected PV value from the flatbuffers message
    """
    assert status_data.Timestamp() > 0
    if expected_value is not None:
        print(f'expected value: {expected_value}, value from message: {status_data.Type()}', flush=True)
        assert expected_value == status_data.Type()


def check_multiple_expected_values(message_list, expected_values):
    """
    Checks for expected PV values in multiple messages.
    Note: not order/time-specific, and requires PVs to have different names.

    :param message_list: A list of flatbuffers objects
    :param expected_values:  A dict with PV names as keys for expected value types and values
    :return: None
    """
    used_pv_names = []
    for log_data in message_list:
        name = str(log_data.SourceName(), encoding='utf-8')
        assert name in expected_values.keys() and name not in used_pv_names
        assert log_data.Timestamp() > 0
        used_pv_names.append(name)
        assert expected_values[name][0] == log_data.ValueType()
        union_val = ValueTypes[log_data.ValueType()]()
        union_val.Init(log_data.Value().Bytes, log_data.Value().Pos)
        print("expected value: {}, value from message: {}".format(expected_values[name][1], union_val.Value())
              , flush=True)
        if isinstance(expected_values[name][1], float):
            isclose(expected_values[name][1], union_val.Value())
        else:
            assert expected_values[name][1] == union_val.Value()


def check_expected_array_values(log_data, value_type, pv_name, expected_value=None):
    """
    Checks the message name (PV) and value type (type of PV), and, optionally, the value.

    :param log_data: Log data object from the received stream buffer
    :param value_type: Flatbuffers value type
    :param pv_name: Byte encoded string of the PV/channel name
    :param expected_value: The expected PV value from the flatbuffers message
    :return: None
    """

    assert value_type == log_data.ValueType()
    assert bytes(pv_name, encoding='utf-8') == log_data.SourceName()
    assert log_data.Timestamp() > 0
    print("expected value type: ", type(expected_value))

    if expected_value is not None:
        print("value type:", value_type)
        union_val = ValueTypes[value_type]()
        print("union val 1 type: ", type(union_val))
        union_val.Init(log_data.Value().Bytes, log_data.Value().Pos)
    if isinstance(union_val, ArrayFloat.ArrayFloat):
        for i in range(union_val.ValueLength()):
            print(i, " ", union_val.Value(i), " ", expected_value[i], "\n")
            assert isclose(union_val.Value(i), expected_value[i], rel_tol=1e-07)
    else:
        arraysmatching = True
        for i in range(union_val.ValueLength()):
            print(i, " ", union_val.Value(i), " ", expected_value[i], "\n")
            if expected_value[i] != union_val.Value(i):
                arraysmatching = False
        assert arraysmatching


