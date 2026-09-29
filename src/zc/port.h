#pragma once
/* The bootstrap compiler runs synchronously on one CPU. Its task context is
 * deliberately separate from the legacy HC runtime and future ZealOS CTask. */
#define _Thread_local
