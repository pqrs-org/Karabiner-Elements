extension krbn_service_state {
  var boolValue: Bool? {
    switch self {
    case krbn_service_state_true: true
    case krbn_service_state_false: false
    default: nil  // Query failures must not be treated as false.
    }
  }
}
