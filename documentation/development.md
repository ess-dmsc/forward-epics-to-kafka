# Information for developers

### Update Frequency

Note that EPICS is not made for very high frequency updates as it will discard updates if there are too many.

That being said, a process variable updated at 10 kHz containing 2048 doubles,
with 3 EPICS to Flatbuffer converters attached and therefore producing 460MB/s
of data works just fine, utilizing about 30% of each core on a reasonable desktop machine.

Higher frequency updates over EPICS should be batched into a PV structure which can hold multiple events at a time, such 
as a waveform record.

The Forwarder uses the [MDEL](https://epics.anl.gov/EpicsDocumentation/AppDevManuals/RecordRef/Recordref-5.html#MARKER-9-15) 
monitor specification for monitoring PV updates rather than the ADEL Archive monitoring specification. This means that 
every PV update is processed rather than just those that exceed the ADEL.

#### Idle PV Updates

To enable the forwarder to publish PV values periodically even if their values have not been updated use the 
`pv-update-period <MILLISECONDS>` flag. This runs alongside the normal PV monitor so it will push value updates as well 
as sending values periodically.

By default this is not enabled.

## Adding New Converter Plugins

New converters from EPICS to Flatbuffers can be easily added.

Please have a look at the last 20 lines of `src/schemas/f142/f142.cxx` on how to register your plugin with the 
SchemaRegistry.
There is no need to touch existing code at all to register a new plugin, but you probably want to add it to `CMakeLists.txt`.

Beware that converter instances are used from different threads.  If the converter instance has state, it must take care 
of thread safety itself.
