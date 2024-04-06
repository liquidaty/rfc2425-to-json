# rfc2425-to-json: utility and C library to convert RFC2425-conforming text to JSON


## Why

This project originated from a need to convert ICS files to a format suitable for
data analysis. For that purpose, any tabular format would suffice.

Existing converters were considered. However, all that we reviewed had one or more
shortcomings that failed to meet our needs, e.g.:

- Security: we needed a solution that does not require exposing / sending data
- Can be used as a utility or a library: requires disclosing source code (and
  permissive license)
- Flexibility: no limits on output columns
- Platform-independent: can run on bare metal on any OS and as web assembly

Because the ICS format is based on [RFC 5545](https://datatracker.ietf.org/doc/html/rfc5545),
which in turn is an extension of [RFC 2425](https://datatracker.ietf.org/doc/html/rfc2425),
an ICS parser would just be a more specialized version of an RFC 2425 parser.

Furthermore, because these formats are not tabular, conversion to tabular data
would be a further specialization of the parser.

In order for a solution to each of these steps (parsing 2425, parsing 5545, converting
to tabular format) to be more re-usable, this library/utility was created, to solve
the first step and optionally also the second step, in a lossless manner, and output
in JSON, a format that can be queried and manipulated using common tools.

The third step (converting non-tabular JSON to tabular format) is left for a separate
solution, such as using [jq](https://github.com/jqlang/jq) to convert to CSV via the filter
`(map(keys) | add | unique) as $cols | map(. as $row | $cols | map($row[.])) as $rows | $cols, ($rows[]|[.[]|if type=="array" or type == "object" then (.|tostring) else . end]) | @csv'`.

For example, the below extracts all calendar events from an ICS file `mycalendar.ics`
and converts to CSV, retaining non-scalar as stringified JSON:

```
rfc2425-to-json mycalendar.ics --ics | jq '.VCALENDAR[0].VEVENT|(map(keys) | add | unique) as $cols | map(. as $row | $cols | map($row[.])) as $rows | $cols, ($rows[]|[.[]|if type=="array" or type == "object" then (.|tostring) else . end]) | @csv' -r
```

For example, to generate a list of US holidays:
```
curl 'https://www.thunderbird.net/media/caldata/autogen/USHolidays.ics' | rfc2425-to-json - --ics | jq '.VCALENDAR[0].VEVENT|(map(keys) | add | unique) as $cols | map(. as $row | $cols | map($row[.])) as $rows | $cols, ($rows[]|[.[]|if type=="array" or type == "object" then (.|tostring) else . end]) | @csv' -r
```

Or phases of the moon:
```
curl 'https://calendar.google.com/calendar/ical/ht3jlfaac5lfd6263ulfh4tql8%40group.calendar.google.com/public/basic.ics' | rfc2425-to-json - --ics | jq '.VCALENDAR[0].VEVENT|(map(keys) | add | unique) as $cols | map(. as $row | $cols | map($row[.])) as $rows | $cols, ($rows[]|[.[]|if type=="array" or type == "object" then (.|tostring) else . end]) | @csv' -r
```

## Building and installing

to build the utility:
  ./configure && make all

to install the utility:
  ./configure && make all && make install

### Library

This code is designed to be compilable into library form, but that feature is not yet
fully implemented.


## Contributing

If you found this utility / library useful or interesting, please give it a star!

* [Fork](https://github.com/liquidaty/rfc2425-to-json/fork) the project.
* Check out the latest [`main`](https://github.com/liquidaty/rfc2425-to-json/tree/main)
  branch.
* Create a feature or bugfix branch from `main`.
* Commit and push your changes.
* Submit the PR.

## License

[MIT](https://github.com/liquidaty/rfc2425-to-json/blob/master/LICENSE)
