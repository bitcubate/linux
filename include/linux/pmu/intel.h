/*
 * Driver for Intel PMU device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _PMU_INTEL_H
#define _PMU_INTEL_H

#if IS_ENABLED(CONFIG_INTEL_PMU)
bool intel_pmu_is_available(void);
#else
static inline bool intel_pmu_is_available(void) { return false; }
#endif

#endif /* _PMU_INTEL_H */
