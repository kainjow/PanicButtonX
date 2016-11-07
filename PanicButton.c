//
//  PanicButton.c
//  PanicButtonX
//
//  Created by Kevin Wojniak on 8/6/10.
//  Copyright 2010 Kevin Wojniak. All rights reserved.
//

#include "PanicButton.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <unistd.h>
#include <block.h>
#include <dispatch/dispatch.h>


#define kPanicButtonVendorID		4400
#define kPanicButtonPrimaryUsage	0
#define kPanicButtonProductName		"Panic Button"

static void (^buttonHandler)();

static void timerCallBack(CFRunLoopTimerRef timer, void *info)
{
	IOHIDDeviceRef device = (IOHIDDeviceRef)info;
	if (device && CFGetTypeID(device) == IOHIDDeviceGetTypeID()) {
		uint8_t data[8];
		CFIndex reportLen = sizeof(data);
		IOReturn ret = IOHIDDeviceGetReport(device, kIOHIDReportTypeFeature, 0, data, &reportLen);
		if (ret == kIOReturnSuccess && data[0])
			dispatch_async(dispatch_get_main_queue(), (dispatch_block_t)buttonHandler);
	}
}

static void hidDeviceRemovedCallback(void *context, IOReturn result, void *sender)
{
	CFRunLoopTimerRef timer = (CFRunLoopTimerRef)context;
	if (timer && CFGetTypeID(timer) == CFRunLoopTimerGetTypeID()) {
		CFRunLoopTimerContext ctx;
		bzero(&ctx, sizeof(ctx));
		CFRunLoopTimerGetContext(timer, &ctx);
		if (ctx.info && CFGetTypeID(ctx.info) == IOHIDDeviceGetTypeID())
			CFRelease(ctx.info);
		CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
		timer = NULL;
	}
}

static void hidDeviceMatchingCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
	CFRunLoopTimerContext ctx;
	bzero(&ctx, sizeof(ctx));
	ctx.info = (void *)CFRetain(device);
	CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault, 0, 0.1, 0, 0, timerCallBack, &ctx);
	if (timer) {
		CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
		IOHIDDeviceRegisterRemovalCallback(device, hidDeviceRemovedCallback, timer);
		CFRelease(timer);
	}
}

CFDictionaryRef createMatchingDictionary()
{
	CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	int numValue = kPanicButtonVendorID;
	CFNumberRef num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &numValue);
	if (num) {
		CFDictionarySetValue(dict, CFSTR(kIOHIDVendorIDKey), num);
		CFRelease(num);
	}
	numValue = kPanicButtonPrimaryUsage;
	num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &numValue);
	if (num) {
		CFDictionarySetValue(dict, CFSTR(kIOHIDPrimaryUsageKey), num);
		CFRelease(num);
	}
	CFDictionarySetValue(dict, CFSTR(kIOHIDProductKey), CFSTR(kPanicButtonProductName));
	return dict;
}

void PanicButtonSetHandler(void (^handler)())
{
	buttonHandler = Block_copy(handler);
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
		CFDictionaryRef dict = createMatchingDictionary();
		if (dict) {
			IOHIDManagerSetDeviceMatching(manager, dict);
			CFRelease(dict);
		}
		IOHIDManagerRegisterDeviceMatchingCallback(manager, hidDeviceMatchingCallback, NULL);
		IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		IOReturn ret = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
		if (ret == kIOReturnSuccess)
			CFRunLoopRun();
		CFRelease(manager);
	});		
}
