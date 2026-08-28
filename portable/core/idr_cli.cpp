#define wmain IdrCliOriginalWmain
#include "idr_cli_impl.cpp"
#undef wmain

#include <cstring>
#include <string>

namespace {

int EstimateLegacyCompatibleEntrySize(const idr::core::LoadedPeImage &image) {
    if (image.entryPoint == 0 || image.bytes.empty()) return 0;

    idr::core::LoadedPeImage &mutableImage =
        const_cast<idr::core::LoadedPeImage &>(image);
    idr::core::ActivateLoadedPeImage(mutableImage);

    MDisasm estimatorDisasm;
    if (!estimatorDisasm.Init()) {
        idr::core::SetImageView({});
        return 0;
    }

    const idr::core::DWord fromAddress = image.entryPoint;
    idr::core::DWord currentAddress = fromAddress;
    idr::core::DWord lastForwardAddress = 0;

    constexpr std::size_t kMaxInstructions = 65536;
    constexpr int kMaxInstructionBytes = 16;

    for (std::size_t row = 0; row < kMaxInstructions; ++row) {
        const int offset = idr::core::AddressToOffset(currentAddress);
        if (offset < 0) {
            idr::core::SetImageView({});
            return 0;
        }
        const auto position = static_cast<std::size_t>(offset);
        if (position >= image.bytes.size() ||
            image.bytes.size() - position < kMaxInstructionBytes) {
            idr::core::SetImageView({});
            return 0;
        }

        DISINFO info{};
        const int length = estimatorDisasm.Disassemble(
            mutableImage.bytes.data() + position,
            static_cast<__int64>(currentAddress),
            &info,
            0);
        if (length <= 0) {
            idr::core::SetImageView({});
            return 0;
        }

        const idr::core::DWord nextAddress =
            currentAddress + static_cast<idr::core::DWord>(length);

        if (lastForwardAddress != 0 && currentAddress >= lastForwardAddress)
            lastForwardAddress = 0;

        if (info.Ret && lastForwardAddress == 0) {
            idr::core::SetImageView({});
            return static_cast<int>(nextAddress - fromAddress);
        }

        if (info.Branch && info.OpType[0] == otIMM) {
            const idr::core::DWord target = info.Immediate;
            const bool targetMapped = idr::core::AddressToOffset(target) >= 0;

            if (info.Conditional) {
                if (targetMapped && target >= fromAddress &&
                    target > lastForwardAddress)
                    lastForwardAddress = target;
            } else {
                if (currentAddress == fromAddress) {
                    idr::core::SetImageView({});
                    return length;
                }
                if ((!targetMapped || target < fromAddress) &&
                    lastForwardAddress == 0) {
                    idr::core::SetImageView({});
                    return static_cast<int>(nextAddress - fromAddress);
                }
                if (targetMapped && target >= fromAddress &&
                    target > lastForwardAddress)
                    lastForwardAddress = target;
            }
        }

        currentAddress = nextAddress;
    }

    idr::core::SetImageView({});
    return 0;
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc == 2) {
        const std::filesystem::path target(argv[1]);
        idr::core::LoadedPeImage image;
        std::string error;
        if (idr::core::LoadPe32File(target, image, &error)) {
            const int estimatedSize = EstimateLegacyCompatibleEntrySize(image);
            if (estimatedSize > 0) {
                std::wstring sizeText = std::to_wstring(estimatedSize);
                wchar_t entrySizeOption[] = L"--entry-size";
                wchar_t *estimatedArgv[] = {
                    argv[0],
                    argv[1],
                    entrySizeOption,
                    sizeText.data()
                };
                std::cout << "entry-size-estimator=legacy-compatible\n";
                std::cout << "entry-size-estimated=" << estimatedSize << '\n';
                return IdrCliOriginalWmain(4, estimatedArgv);
            }
        }
    }

    return IdrCliOriginalWmain(argc, argv);
}
