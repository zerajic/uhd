//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "libusb1_base.hpp"
#include <uhd/utils/assert.hpp>
#include <iostream>

using namespace uhd::transport;

/**********************************************************
 * Helper Methods
 **********************************************************/

/**********************************************************
 * libusb namespace 
 **********************************************************/
void libusb::init(libusb_context **ctx, int debug_level)
{
    if (libusb_init(ctx) < 0)
        std::cerr << "error: libusb_init" << std::endl;

    libusb_set_debug(*ctx, debug_level);
}

libusb_device_handle *libusb::open_device(libusb_context *ctx,
                                          usb_device_handle::sptr handle)
{
    libusb_device_handle *dev_handle = NULL;
    libusb_device **libusb_dev_list;
    size_t dev_cnt = libusb_get_device_list(ctx, &libusb_dev_list);

    //find and open the USB device 
    for (size_t i = 0; i < dev_cnt; i++) {
        libusb_device *dev = libusb_dev_list[i];

        if (compare_device(dev, handle)) {
            libusb_open(dev, &dev_handle);
            libusb_unref_device(dev);
            break;
        }
            
        libusb_unref_device(dev);
    }

    return dev_handle;
}

//note: changed order of checks so it only tries to get_serial and get_device_address if vid and pid match
//doing this so it doesn't try to open the device if it's not ours
bool libusb::compare_device(libusb_device *dev,
                            usb_device_handle::sptr handle)
{
    std::string serial         = handle->get_serial();
    boost::uint16_t vendor_id  = handle->get_vendor_id();
    boost::uint16_t product_id = handle->get_product_id();
    boost::uint8_t device_addr = handle->get_device_addr();

    libusb_device_descriptor libusb_desc;
    if (libusb_get_device_descriptor(dev, &libusb_desc) < 0)
        return false;
    if (vendor_id != libusb_desc.idVendor)
        return false;
    if (product_id != libusb_desc.idProduct)
        return false; 
    if (serial != get_serial(dev))
        return false;
    if (device_addr != libusb_get_device_address(dev))
        return false;

    return true;
}


bool libusb::open_interface(libusb_device_handle *dev_handle,
                            int interface)
{
    int ret = libusb_claim_interface(dev_handle, interface);
    if (ret < 0) {
        std::cerr << "error: libusb_claim_interface() " << ret << std::endl;
        return false;
    }
    else {
        return true;
    }
}


std::string libusb::get_serial(libusb_device *dev)
{
    unsigned char buff[32];

    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(dev, &desc) < 0)
        return "";

    if (desc.iSerialNumber == 0)
        return "";

    //open the device because we have to
    libusb_device_handle *dev_handle;
    if (libusb_open(dev, &dev_handle) < 0)
        return "";

    if (libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber,
                                           buff, sizeof(buff)) < 0) {
        return "";
    }

    libusb_close(dev_handle);

    return (char*) buff;
}