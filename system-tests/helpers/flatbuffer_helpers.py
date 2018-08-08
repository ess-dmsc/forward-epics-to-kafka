from helpers.f142_logdata import Int, Double, String, Long, Value

ValueTypes = {
    Value.Value.Int: Int.Int,
    Value.Value.Double: Double.Double,
    Value.Value.String: String.String,
}


def check_expected_values(log_data, value_type, pv_name, expected_value="None"):
    """
    Checks the message name (PV) and value type (type of PV).

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
