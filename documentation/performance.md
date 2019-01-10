## Performance

Some more thorough figures are to be included here, but we have forwarded
about 200MB/s without problems for extended periods of time.
Higher bandwidth has been done, but not yet tested over long time periods.

### Conversion bandwidth

If we run as usual except that we do not actually write to Kafka, tests show
on my quite standard 4 core desktop PC a Flatbuffer conversion rate of about
2.8 GB/s. with all cores at a ~80% usage.
These numbers are of course very rough estimates and depend on a lot of
factors.  For systematic tests are to be done.
