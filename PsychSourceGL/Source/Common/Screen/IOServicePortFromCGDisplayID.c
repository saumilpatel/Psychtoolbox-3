//
//  CGDisplayIOServicePort.c
//  
//
//  Created by Saumil on 11/19/19.
//
#include <IOKit/graphics/IOGraphicsLib.h>
#include <ApplicationServices/ApplicationServices.h>

static io_service_t IOServicePortFromCGDisplayID(CGDirectDisplayID displayID)
{
    io_iterator_t iter;
    io_service_t serv, servicePort = 0;
    
    CFMutableDictionaryRef matching = IOServiceMatching("IODisplayConnect");
    
    // releases matching for us
    kern_return_t err = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                     matching,
                                                     &iter);
    if (err)
        return 0;
    
    while ((serv = IOIteratorNext(iter)) != 0)
    {
        CFDictionaryRef info;
        CFIndex vendorID, productID, serialNumber;
        CFNumberRef vendorIDRef, productIDRef, serialNumberRef;
        Boolean success;
        
        info = IODisplayCreateInfoDictionary(serv,
                                             kIODisplayOnlyPreferredName);
        
        vendorIDRef = CFDictionaryGetValue(info,
                                           CFSTR(kDisplayVendorID));
        productIDRef = CFDictionaryGetValue(info,
                                            CFSTR(kDisplayProductID));
        /*
         serialNumberRef = CFDictionaryGetValue(info,
         CFSTR(kDisplaySerialNumber));
         */
        
        success = CFNumberGetValue(vendorIDRef, kCFNumberCFIndexType,
                                   &vendorID);
        success &= CFNumberGetValue(productIDRef, kCFNumberCFIndexType,
                                    &productID);
        success &= CFNumberGetValue(serialNumberRef, kCFNumberCFIndexType,
                                    &serialNumber);
        
        if (!success)
        {
            CFRelease(info);
            continue;
        }
        
        // If the vendor and product id along with the serial don't match
        // then we are not looking at the correct monitor.
        // NOTE: The serial number is important in cases where two monitors
        //       are the exact same.
        if (CGDisplayVendorNumber(displayID) != vendorID  ||
            CGDisplayModelNumber(displayID) != productID  ||
            CGDisplaySerialNumber(displayID) != serialNumber)
        {
            CFRelease(info);
            continue;
        }
        
        // The VendorID, Product ID, and the Serial Number all Match Up!
        // Therefore we have found the appropriate display io_service
        servicePort = serv;
        CFRelease(info);
        break;
    }
    
    IOObjectRelease(iter);
    return servicePort;
}


// Get the name of the specified display
//
static char* getDisplayName(CGDirectDisplayID displayID)
{
    char* name;
    CFDictionaryRef info, names;
    CFStringRef value;
    CFIndex size;
    
    // Supports OS X 10.4 Tiger and Newer
    io_service_t serv = IOServicePortFromCGDisplayID(displayID);
    if (serv == 0)
    {
        /*
         _glfwInputError(GLFW_PLATFORM_ERROR,
         "Cocoa: IOServicePortFromCGDisplayID Returned an Invalid Port. (Port: 0)");
         */
        return strdup("Unknown");
    }
    
    info = IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName);
    IOObjectRelease(serv);
    
    names = CFDictionaryGetValue(info, CFSTR(kDisplayProductName));
    
    if (!names || !CFDictionaryGetValueIfPresent(names, CFSTR("en_US"),
                                                 (const void**) &value))
    {
        // This may happen if a desktop Mac is running headless
        /*
         _glfwInputError(GLFW_PLATFORM_ERROR,
         "Cocoa: Failed to retrieve display name");
         */
        
        CFRelease(info);
        return strdup("Unknown");
    }
    
    size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(value),
                                             kCFStringEncodingUTF8);
    name = calloc(size + 1, sizeof(char));
    CFStringGetCString(value, name, size, kCFStringEncodingUTF8);
    
    CFRelease(info);
    
    return name;
}

