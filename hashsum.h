#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pthread.h>
#include <unistd.h>

#include "constants.h"
#include "types.h"

void hashGenerator(char *salt, char *password, char *hash);
