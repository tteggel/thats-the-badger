#include <cstring>
#include "pico/platform.h"

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "state.hpp"

#define FLASH_TARGET_OFFSET (256 * 1024)

void store_state(const State* data)
{
  uint32_t len = sizeof(State);

  if (len > FLASH_BLOCK_SIZE) len = FLASH_BLOCK_SIZE;

  const uint32_t page_count = (len / FLASH_PAGE_SIZE) + 1;
  const uint32_t sector_count = ((page_count * FLASH_PAGE_SIZE) / FLASH_SECTOR_SIZE) + 1;

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE * sector_count);
  flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *) data, FLASH_PAGE_SIZE * page_count);
  restore_interrupts(ints);
}

void get_state(State* state)
{
  const auto* data = (const State*)(XIP_BASE + FLASH_TARGET_OFFSET);
  if (data->magic == 0x6022) {
    memcpy(state, data, sizeof(State));
  }
}
