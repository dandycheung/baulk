// exports
#include "internal.hpp"
#include <algorithm>

namespace bela::pe {
bool File::LookupExports(std::vector<ExportedSymbol> &exports, bela::error_code &ec) const {
  auto exd = getDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
  if (exd == nullptr) {
    return true;
  }
  auto ds = getSection(exd);
  if (ds == nullptr) {
    return true;
  }
  auto sdata = readSectionData(*ds, ec);
  if (!sdata) {
    return false;
  }
  auto bv = sdata->as_bytes_view();
  // seek to the virtual address specified in the export data directory
  auto N = exd->VirtualAddress - ds->VirtualAddress;
  auto cied = bv.checked_cast<IMAGE_EXPORT_DIRECTORY>(N);
  if (cied == nullptr) {
    return true;
  }
  IMAGE_EXPORT_DIRECTORY ied;
  if constexpr (bela::IsLittleEndian()) {
    memcpy(&ied, cied, sizeof(IMAGE_EXPORT_DIRECTORY));
  } else {
    ied.Characteristics = bela::fromle(cied->Characteristics);
    ied.TimeDateStamp = bela::fromle(cied->TimeDateStamp);
    ied.MajorVersion = bela::fromle(cied->MajorVersion);
    ied.MinorVersion = bela::fromle(cied->MinorVersion);
    ied.Name = bela::fromle(cied->Name);
    ied.Base = bela::fromle(cied->Base);
    ied.NumberOfFunctions = bela::fromle(cied->NumberOfFunctions);
    ied.NumberOfNames = bela::fromle(cied->NumberOfNames);
    ied.AddressOfFunctions = bela::fromle(cied->AddressOfFunctions);       // RVA from base of image
    ied.AddressOfNames = bela::fromle(cied->AddressOfNames);               // RVA from base of image
    ied.AddressOfNameOrdinals = bela::fromle(cied->AddressOfNameOrdinals); // RVA from base of image
  }
  if (ied.NumberOfFunctions == 0 || ied.AddressOfFunctions == 0) {
    return true;
  }
  const auto sectionEnd = ds->VirtualAddress + ds->VirtualSize;
  const auto exdEnd = exd->VirtualAddress + exd->Size;
  auto ordinalBase = static_cast<uint16_t>(ied.Base);
  exports.resize(ied.NumberOfFunctions);
  auto ordinalTable = bv.subview(ied.AddressOfNameOrdinals - ds->VirtualAddress);
  auto nameTable = bv.subview(ied.AddressOfNames - ds->VirtualAddress);
  auto funTable = bv.subview(ied.AddressOfFunctions - ds->VirtualAddress);
  auto addressTable = bv.subview(ied.AddressOfFunctions - ds->VirtualAddress);
  for (DWORD i = 0; i < ied.NumberOfFunctions; i++) {
    auto address = addressTable.cast_fromle<uint32_t>(i * 4);
    if (address > exd->VirtualAddress && address < exdEnd) {
      exports[i].ForwardName = bv.make_cstring_view(address - ds->VirtualAddress);
    }
    exports[i].Address = address;
    exports[i].Ordinal = static_cast<uint16_t>(i + ordinalBase);
  }
  for (DWORD i = 0; i < ied.NumberOfNames; i++) {
    auto nameRVA = nameTable.cast_fromle<uint32_t>(i * 4);
    auto name = bv.make_cstring_view(nameRVA - ds->VirtualAddress);
    uint16_t ordinal = ordinalTable.cast_fromle<uint16_t>(i * 2) + ordinalBase;
    auto index = ordinal - ordinalBase;
    if (index < 0 || static_cast<DWORD>(index) > ied.NumberOfFunctions) {
      continue;
    }
    exports[index].Name = name;
    exports[index].Hint = i;
  }

  std::sort(exports.begin(), exports.end(), [](const ExportedSymbol &a, const ExportedSymbol &b) -> bool {
    //
    return a.Ordinal < b.Ordinal;
  });
  return true;
}

} // namespace bela::pe