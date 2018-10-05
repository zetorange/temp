import CCateyes

@objc(CateyesSpawnDetails)
public class SpawnDetails: NSObject, NSCopying {
    private let handle: OpaquePointer

    init(handle: OpaquePointer) {
        self.handle = handle
    }

    public func copy(with zone: NSZone?) -> Any {
        g_object_ref(gpointer(handle))
        return SpawnDetails(handle: handle)
    }

    deinit {
        g_object_unref(gpointer(handle))
    }

    public var pid: UInt32 {
        return cateyes_spawn_get_pid(handle)
    }

    public var identifier: String? {
        return String(cString: cateyes_spawn_get_identifier(handle))
    }

    public override var description: String {
        if let identifier = self.identifier {
            return "Cateyes.SpawnDetails(pid: \(pid), identifier: \"\(identifier)\")"
        } else {
            return "Cateyes.SpawnDetails(pid: \(pid))"
        }
    }

    public override func isEqual(_ object: Any?) -> Bool {
        if let details = object as? SpawnDetails {
            return details.handle == handle
        } else {
            return false
        }
    }

    public override var hash: Int {
        return handle.hashValue
    }
}
