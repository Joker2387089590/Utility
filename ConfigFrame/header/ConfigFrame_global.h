#pragma once
#include <QtCore/qglobal.h>

#if defined(CONFIGFRAME_LIBRARY)
#  define CONFIGFRAME_EXPORT Q_DECL_EXPORT
#else
#  define CONFIGFRAME_EXPORT Q_DECL_IMPORT
#endif
