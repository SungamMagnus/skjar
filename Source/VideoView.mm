#include "VideoView.h"

#include "YouTube.h"

#import <WebKit/WebKit.h>

namespace skjar
{

namespace
{
NSString* toNS (const juce::String& s) { return [NSString stringWithUTF8String: s.toRawUTF8()]; }
juce::String fromNS (NSString* s)      { return juce::String::fromUTF8 ([s UTF8String]); }
} // namespace

struct VideoView::Impl
{
    WKWebView* web = nil;
};

VideoView::VideoView() : impl_ (std::make_unique<Impl>())
{
    WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
    // The transport starts the video, not a click inside it.
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;

    impl_->web = [[WKWebView alloc] initWithFrame: NSMakeRect (0, 0, 640, 360)
                                    configuration: config];
    [config release];

    // YouTube serves a degraded page, or none, to user agents it does not know.
    impl_->web.customUserAgent = @"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                                  "AppleWebKit/605.1.15 (KHTML, like Gecko) "
                                  "Version/18.0 Safari/605.1.15";
    [impl_->web setValue: @NO forKey: @"drawsBackground"];

    setView (impl_->web);   // retains it
    clear();
}

VideoView::~VideoView()
{
    [impl_->web stopLoading];
    setView (nullptr);
    [impl_->web release];
}

void VideoView::load (const juce::String& videoId, double startSeconds)
{
    state_ = {};
    const auto page = youtube::playerPage (videoId, startSeconds);
    [impl_->web loadHTMLString: toNS (page)
                       baseURL: [NSURL URLWithString: @(youtube::pageOrigin)]];
}

void VideoView::clear()
{
    state_ = {};
    [impl_->web loadHTMLString: @"<html><body style='margin:0;background:#201f1e'></body></html>"
                       baseURL: nil];
}

void VideoView::run (const juce::String& js)
{
    [impl_->web evaluateJavaScript: toNS (js) completionHandler: nil];
}

void VideoView::start (double seconds) { run ("skjarStart(" + juce::String (seconds, 3) + ")"); }
void VideoView::park (double seconds)  { run ("skjarPark(" + juce::String (seconds, 3) + ")"); }
void VideoView::seek (double seconds)  { run ("skjarSeek(" + juce::String (seconds, 3) + ")"); }
void VideoView::play()                 { run ("skjarPlay()"); }

void VideoView::setMuted (bool muted)
{
    muted_ = muted;
    run (muted ? "skjarMute(true)" : "skjarMute(false)");
}

double VideoView::stateAge() const
{
    return juce::Time::getMillisecondCounterHiRes() * 0.001 - stateStamp_;
}

void VideoView::poll()
{
    juce::WeakReference<VideoView> self (this);
    [impl_->web evaluateJavaScript: @"typeof skjarState === 'function' ? skjarState() : null"
                 completionHandler: ^(id result, NSError*) {
        if (self == nullptr || ! [result isKindOfClass: [NSString class]])
            return;

        const auto json = juce::JSON::parse (fromNS ((NSString*) result));
        PlayerState s;
        s.ready = json["ready"];
        s.error = json["error"];
        if (s.ready)
        {
            s.state = json["state"];
            s.time = json["time"];
            s.duration = json["duration"];
        }

        // The page forgets it was muted every time it reloads.
        if (s.ready && ! self->state_.ready)
            self->setMuted (self->muted_);

        self->state_ = s;
        self->stateStamp_ = juce::Time::getMillisecondCounterHiRes() * 0.001;
    }];
}

} // namespace skjar
