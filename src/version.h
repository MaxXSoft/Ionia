#ifndef IONIA_VERSION_H_
#define IONIA_VERSION_H_

// version information of Ionia

#ifndef APP_NAME
#define APP_NAME            "Ionia"
#endif

#ifndef APP_VERSION
#define APP_VERSION         "0.0.0"
#endif

#ifndef APP_VERSION_MAJOR
#define APP_VERSION_MAJOR   0
#endif

#ifndef APP_VERSION_MINOR
#define APP_VERSION_MINOR   0
#endif

#ifndef APP_VERSION_PATCH
#define APP_VERSION_PATCH   0
#endif

// compare given version with the current version
// returns:
//    negative: less than
//    zero:     equal
//    positive: greater than
inline int CompareVersion(int major, int minor, int patch) {
  int tar = major * 1000000 + minor * 10000 + patch;
  int cur = APP_VERSION_MAJOR * 1000000;
  cur += APP_VERSION_MINOR * 10000 + APP_VERSION_PATCH;
  return tar - cur;
}

#endif  // IONIA_VERSION_H_
