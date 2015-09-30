misc-non-copyable-objects
=========================

The check flags dereferences and non-pointer declarations of objects that are
not meant to be passed by value, such as C FILE objects or POSIX
pthread_mutex_t objects.

This check corresponds to CERT C++ Coding Standard rule `FIO38-C. Do not copy a FILE object
<https://www.securecoding.cert.org/confluence/display/c/FIO38-C.+Do+not+copy+a+FILE+object>`_.
