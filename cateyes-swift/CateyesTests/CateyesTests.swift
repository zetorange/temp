import XCTest
@testable import Cateyes

class CateyesTests: XCTestCase {

    override func setUp() {
        super.setUp()
    }

    override func tearDown() {
        super.tearDown()
    }

    func testEnumerateDevices() {
        let expectation = self.expectation(description: "Got list of devices")

        let manager = DeviceManager()
        var devices = [Device]()
        manager.enumerateDevices { result in
            devices = try! result()
            expectation.fulfill()
        }
        manager.close()

        self.waitForExpectations(timeout: 5.0, handler: nil)
        // print("Got devices: \(devices)")
        assert(devices.count > 0)
    }

    func testGetFrontmostApplication() {
        let expectation = self.expectation(description: "Got frontmost application")

        let manager = DeviceManager()
        var application: ApplicationDetails?
        manager.enumerateDevices { result in
            let devices = try! result()
            let usbDevice = devices.filter { $0.kind == Device.Kind.usb }.first!
            usbDevice.getFrontmostApplication() { result in
                application = try! result()
                expectation.fulfill()
            }
        }

        self.waitForExpectations(timeout: 5.0, handler: nil)
        print("Got application: \(application.debugDescription)")
    }

    func testEnumerateApplications() {
        let expectation = self.expectation(description: "Got list of applications")

        let manager = DeviceManager()
        var applications = [ApplicationDetails]()
        manager.enumerateDevices { result in
            let devices = try! result()
            let usbDevice = devices.filter { $0.kind == Device.Kind.usb }.first!
            usbDevice.enumerateApplications() { result in
                applications = try! result()
                expectation.fulfill()
            }
        }

        self.waitForExpectations(timeout: 5.0, handler: nil)
        // print("Got applications: \(applications)")
        assert(applications.count > 0)
    }

    func testEnumerateProcesses() {
        let expectation = self.expectation(description: "Got list of processes")

        let manager = DeviceManager()
        var processes = [ProcessDetails]()
        manager.enumerateDevices { result in
            let devices = try! result()
            let localDevice = devices.filter { $0.kind == Device.Kind.local }.first!
            localDevice.enumerateProcesses() { result in
                processes = try! result()
                expectation.fulfill()
            }
        }

        self.waitForExpectations(timeout: 5.0, handler: nil)
        // print("Got processes: \(processes)")
        assert(processes.count > 0)
    }

    func xxxtestDetached() {
        let pid: UInt = 63626

        let expectation = self.expectation(description: "Got detached from session")

        class TestDelegate : SessionDelegate {
            let expectation: XCTestExpectation
            var reason: SessionDetachReason?

            init(expectation: XCTestExpectation) {
                self.expectation = expectation
            }

            func session(_ session: Session, didDetach reason: SessionDetachReason) {
                self.reason = reason
                expectation.fulfill()
            }
        }
        let delegate = TestDelegate(expectation: expectation)

        let manager = DeviceManager()
        var session: Session? = nil
        manager.enumerateDevices { result in
            let devices = try! result()
            let localDevice = devices.filter { $0.kind == Device.Kind.local }.first!
            localDevice.attach(pid) { result in
                session = try! result()
                session!.delegate = delegate
            }
        }

        self.waitForExpectations(timeout: 60.0, handler: nil)
        print("Got detached from \(session!), reason: \(delegate.reason!)")
    }

    func xxxtestFullCycle() {
        let pid: UInt = 20854

        let expectation = self.expectation(description: "Got message from script")

        class TestDelegate : ScriptDelegate {
            let expectation: XCTestExpectation
            var messages = [Any]()

            init(expectation: XCTestExpectation) {
                self.expectation = expectation
            }

            func scriptDestroyed(_: Script) {
                print("destroyed")
            }

            func script(_: Script, didReceiveMessage message: Any, withData data: Data?) {
                print("didReceiveMessage")
                messages.append(message)
                if messages.count == 2 {
                    expectation.fulfill()
                }
            }
        }
        let delegate = TestDelegate(expectation: expectation)

        let manager = DeviceManager()
        var script: Script? = nil
        manager.enumerateDevices { result in
            let devices = try! result()
            let localDevice = devices.filter { $0.kind == Device.Kind.local }.first!
            localDevice.attach(pid) { result in
                let session = try! result()
                session.createScript("test", source: "console.log(\"hello\"); send(1337);") { result in
                    let s = try! result()
                    s.delegate = delegate
                    s.load() { result in
                        _ = try! result()
                        print("Script loaded")
                    }
                    script = s
                }
            }
        }

        self.waitForExpectations(timeout: 5.0, handler: nil)
        print("Done with script \(script.debugDescription), messages: \(delegate.messages)")
    }
}
