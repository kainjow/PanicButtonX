//
//  AppController.m
//  PanicButtonX
//
//  Created by Kevin Wojniak on 8/6/10.
//  Copyright 2010 Kevin Wojniak. All rights reserved.
//

#import "AppController.h"
#import "PanicButton.h"


@interface PathTildeTransfomer : NSValueTransformer
@end

@implementation PathTildeTransfomer

- (id)transformedValue:(id)value
{
	return [value stringByAbbreviatingWithTildeInPath];
}

@end


#define PrefsKeyAppleScriptPath	@"AppleScriptPath"
#define PrefsKeyRunAppleScript	@"RunAppleScript"

@implementation AppController

- (void)applicationDidFinishLaunching:(NSNotification *)notif
{
	PanicButtonSetHandler(^{
		if ([[NSUserDefaults standardUserDefaults] boolForKey:PrefsKeyRunAppleScript]) {
			NSString *path = [[NSUserDefaults standardUserDefaults] objectForKey:PrefsKeyAppleScriptPath];
			if (path) {
				NSAppleScript *script = [[[NSAppleScript alloc] initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil] autorelease];
				[script executeAndReturnError:nil];
			}
		}
	});
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

- (IBAction)chooseScript:(id)sender
{
	NSOpenPanel *op = [NSOpenPanel openPanel];
	[op setAllowedFileTypes:[NSArray arrayWithObject:@"scpt"]];
	[op beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result){
		if (result == NSFileHandlingPanelOKButton) {
			[[NSUserDefaults standardUserDefaults] setObject:[op filename] forKey:PrefsKeyAppleScriptPath];
			[[NSUserDefaults standardUserDefaults] setBool:YES forKey:PrefsKeyRunAppleScript];
		};
	}];
}

@end
