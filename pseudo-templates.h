#ifndef TEMPLATES_H
#define TEMPLATES_H

#define CONCATENATE(A, B) A##_##B
#define TEMPLATE(func, TYPE) CONCATENATE(func, TYPE)

#endif
