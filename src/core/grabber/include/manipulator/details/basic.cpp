#include "basic.hpp"
#include "device_grabber.hpp"
#include "types.hpp"
#include "logger.hpp"

using namespace krbn::manipulator::details;

std::pair<uint32_t, uint32_t> basic::get_vendor_product_id_by_device_id(krbn::device_id id) {
  uint32_t vid = 0;
  uint32_t pid = 0;
  
  auto hid = krbn::device_grabber::get_grabber()->get_hid_by_id(id);
  if (hid) {
    auto hid_ptr = (*hid).lock();
    
    vid = static_cast<uint32_t>(hid_ptr->get_vendor_id().value_or(krbn::vendor_id::zero));
    pid = static_cast<uint32_t>(hid_ptr->get_product_id().value_or(krbn::product_id::zero));
  }
  
  return std::make_pair(vid, pid);
}

bool basic::is_bypass_vendor_product_id_check() {
  bool not_to_check = this->vendor_id_ == 0 && this->product_id_ == 0;
  
  if (not_to_check) {
    krbn::logger::get_logger().debug("bypass vendor/product id check");
  }
  return not_to_check;
}

bool basic::is_vendor_product_id_matched(uint32_t vendor_id, uint32_t product_id) {
  bool is_matched = (vendor_id == this->vendor_id_ && product_id == this->product_id_);
  
  if (is_matched) {
    krbn::logger::get_logger().debug("vendor/product id matched");
  } else {
    krbn::logger::get_logger().debug("vendor/product id NOT matched: [{}, {}], [{}, {}]", vendor_id, product_id, this->vendor_id_, this->product_id_);
  }
  
  return is_matched;
}

