from helpers.f142_logdata import Int, Double, String, Long, Value

ValueTypes = {
    Value.Value.Int: Int.Int,
    Value.Value.Double: Double.Double,
    Value.Value.String: String.String,
}


def check_expected_values(log_data, value_type, pv_name, expected_value="None"):
    """
    Checks the message name (PV) and value type (type of PV).

    This function is used for checking values where order should be retained or there is no need to check for an
    expected value.

    :param expected_value: The expected PV value from the flatbuffers message
    :param log_data: Log data object from the received stream buffer
    :param value_type: Flatbuffers value type
    :param pv_name: Byte encoded string of the PV/channel name
    :return: none
    """
    assert value_type == log_data.ValueType()
    assert bytes(pv_name, encoding='utf-8') == log_data.SourceName()
    if expected_value != "None":
        union_val = ValueTypes[value_type]()
        union_val.Init(log_data.Value().Bytes, log_data.Value().Pos)
        assert expected_value == union_val.Value()


def check_expected_values_multiple(message_list, expected_values):
    """
    Checks for expected PV values in multiple messages
    Note: not order/time-specific, and requires PVs to have different names

    :param message_list: A list of flatbuffers objects
    :param expected_values:  A dict with PV names as keys for expected value types and values
    :return: none
    """
    used_pv_names = []
    for log_data in message_list:
        name = str(log_data.SourceName(), encoding='utf-8')
        assert name in expected_values.keys() and name not in used_pv_names
        used_pv_names.append(name)
        assert expected_values[name][0] == log_data.ValueType()
        union_val = ValueTypes[log_data.ValueType()]()
        union_val.Init(log_data.Value().Bytes, log_data.Value().Pos)
        assert expected_values[name][1] == union_val.Value()
