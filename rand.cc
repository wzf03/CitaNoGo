#include "rand.h"

#include <random>
namespace cita {
std::random_device rd;
std::default_random_engine re(rd());
std::uniform_int_distribution<int> dis(0);
int randInt() { return dis(re); }
std::default_random_engine getDefaultRD() { return re; }
}  // namespace cita