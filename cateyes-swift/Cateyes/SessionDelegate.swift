@objc(CateyesSessionDelegate)
public protocol SessionDelegate {
    func session(_ session: Session, didDetach reason: SessionDetachReason)
}
