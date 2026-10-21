#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * The Sungam system in Beygla's graphite edition: paper and ink trade places,
 * so every rule written in terms of them — hairlines at partial ink, fills on
 * paper — still holds, and the picture is not framed in glare.
 *
 * Colour is signal and never decoration. Skjar's assignment:
 *
 *   teal    the source — the link and the picture it brings in
 *   coral   timing — the cue and how the video follows the transport
 *   steel   presets — everything that stores the other two
 *   amber   the live state, and nothing else
 */
namespace skjar::sungam
{

namespace hue
{
const juce::Colour paper { 0xff2e2d2b };
const juce::Colour ink   { 0xffece8df };
const juce::Colour stage { 0xff201f1e };   // behind the picture only
const juce::Colour coral { 0xffed8159 };
const juce::Colour teal  { 0xff52b0a4 };
const juce::Colour steel { 0xff6695c2 };
const juce::Colour amber { 0xffd9a329 };
}

inline juce::Colour ink (float alpha) { return hue::ink.withAlpha (alpha); }

constexpr float hairline = 1.0f, borderDefault = 1.2f;

juce::Font mono (float size, bool bold = false);

/** Letter-spaced caps — the product name, and nothing else. */
void wordmark (juce::Graphics&, const juce::String&, juce::Rectangle<float>, float size,
               juce::Colour);

/** A section, drawn as the box its controls sit inside, with the title inset
    into the top rule rather than captioned above it. */
void frame (juce::Graphics&, juce::Rectangle<float>, juce::Colour, const juce::String& title);

/** Lit when something is happening, outlined when it is not. */
void lamp (juce::Graphics&, juce::Point<float> centre, bool on, juce::Colour);

/** Flat, square, hairline: text fields, latches, the preset menu. A button
    takes its section colour from TextButton::buttonColourId. */
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;

    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int,
                       juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;

    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowTitleFont() override;
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>&,
                       juce::TextLayout&) override;
};

} // namespace skjar::sungam
