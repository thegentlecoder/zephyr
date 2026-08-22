/*
 * Copyright 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_COMMON_H
#define MOCK_COMMON_H

#include <stdint.h>

#ifdef __used
#undef __used
#endif

/* Define mock identifiers and types to make the code compile */
#if !defined(UNUSED)
#define UNUSED(x) ((void)(x))
#endif

#endif /* MOCK_COMMON_H */
