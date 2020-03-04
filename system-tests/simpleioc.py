from caproto.server import pvproperty, PVGroup, ioc_arg_parser, run


class SimpleIOC(PVGroup):
    DOUBLE = pvproperty(value=0)
    DOUBLE2 = pvproperty(value=0)
    DOUBLE3 = pvproperty(value=0)
    FLOAT = pvproperty(value=0)
    STR = pvproperty(value="test")


if __name__ == '__main__':
    ioc_options, run_options = ioc_arg_parser(
        default_prefix='SIMPLE:',
        desc="")
    ioc = SimpleIOC(**ioc_options)
    run(ioc.pvdb, **run_options)
