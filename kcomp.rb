class Kcomp < Formula
  desc "High-performance compression utility with adaptive algorithm selection"
  homepage "https://github.com/khaledalam/kcomp"
  url "https://github.com/khaledalam/kcomp/archive/refs/tags/v1.0.0.tar.gz"
  sha256 "2bafacaca07b67ca632a341964ac1792e5e75f698c27b9ee72bcd3aa45037358"
  license "MIT"

  depends_on "cmake" => :build

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    bin.install "build/kcomp"
  end

  test do
    # Test compression and decompression
    (testpath/"test.txt").write("Hello, World!")
    system "#{bin}/kcomp", "c", "test.txt", "test.kc"
    system "#{bin}/kcomp", "d", "test.kc", "test_out.txt"
    assert_equal "Hello, World!", (testpath/"test_out.txt").read
  end
end
