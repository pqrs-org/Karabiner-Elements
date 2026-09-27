all:
	mkdir -p build
	xcrun swiftc -module-cache-path build/ModuleCache -swift-version 6 -warnings-as-errors -parse-as-library \
		../../../src/apps/share/swift/Concurrency/*.swift \
		test.swift -o build/swift_concurrency_test
	build/swift_concurrency_test

clean:
	rm -rf build
