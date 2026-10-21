#include "Sungam.h"

namespace skjar::sungam
{

juce::Font mono (float size, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

void wordmark (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
               juce::Colour c)
{
    g.setColour (c);
    g.setFont (mono (size, true).withExtraKerningFactor (0.28f));
    g.drawText (s.toUpperCase(), r, juce::Justification::centredLeft, false);
}

void frame (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, const juce::String& title)
{
    g.setColour (c);
    g.drawRect (r, hairline);

    const auto font = mono (10.5f);
    const auto caption = title.toUpperCase();
    const float w = juce::GlyphArrangement::getStringWidth (font, caption) + 12.0f;
    const juce::Rectangle<float> tab { r.getX() + 14.0f, r.getY() - 7.0f, w, 14.0f };

    g.setColour (hue::paper);
    g.fillRect (tab);
    g.setColour (c);
    g.setFont (font);
    g.drawText (caption, tab, juce::Justification::centred, false);
}

void lamp (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour colour)
{
    const auto r = juce::Rectangle<float> (9.0f, 9.0f).withCentre (c);
    g.setColour (colour);
    if (on)
        g.fillEllipse (r);
    else
        g.drawEllipse (r.reduced (0.6f), borderDefault);
}

LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, hue::paper);

    setColour (juce::TextEditor::backgroundColourId, hue::paper);
    setColour (juce::TextEditor::textColourId, hue::ink);
    setColour (juce::TextEditor::highlightColourId, ink (0.22f));
    setColour (juce::TextEditor::highlightedTextColourId, hue::ink);
    setColour (juce::TextEditor::outlineColourId, ink (0.22f));
    setColour (juce::TextEditor::focusedOutlineColourId, ink (0.55f));
    setColour (juce::CaretComponent::caretColourId, hue::ink);

    setColour (juce::TextButton::buttonColourId, hue::steel);
    setColour (juce::TextButton::textColourOffId, ink (0.62f));
    setColour (juce::TextButton::textColourOnId, hue::paper);

    setColour (juce::ComboBox::backgroundColourId, hue::paper);
    setColour (juce::ComboBox::textColourId, hue::ink);
    setColour (juce::ComboBox::outlineColourId, ink (0.22f));
    setColour (juce::ComboBox::arrowColourId, ink (0.55f));

    setColour (juce::PopupMenu::backgroundColourId, hue::paper);
    setColour (juce::PopupMenu::textColourId, hue::ink);
    setColour (juce::PopupMenu::headerTextColourId, ink (0.55f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ink (0.13f));
    setColour (juce::PopupMenu::highlightedTextColourId, hue::ink);

    setColour (juce::AlertWindow::backgroundColourId, hue::paper);
    setColour (juce::AlertWindow::textColourId, hue::ink);
    setColour (juce::AlertWindow::outlineColourId, ink (0.32f));

    setColour (juce::Label::textColourId, hue::ink);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return mono (10.5f, true);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                        bool over, bool down)
{
    // A latch: filled in its colour when on, outlined when off. A momentary
    // button is a latch that is only on while held.
    const auto c = b.findColour (juce::TextButton::buttonColourId);
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState() || down;

    g.setColour (on ? c : (over ? ink (0.08f) : hue::paper));
    g.fillRect (r);
    g.setColour (c.withAlpha (on || over ? 1.0f : 0.55f));
    g.drawRect (r, borderDefault);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const bool on = b.getToggleState() || down;
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (on ? hue::paper : ink (b.isEnabled() ? 0.70f : 0.32f));
    g.drawText (b.getButtonText().toUpperCase(), b.getLocalBounds(), juce::Justification::centred, false);
}

void LookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.findColour (juce::TextEditor::backgroundColourId));
    g.fillRect (0, 0, w, h);
}

void LookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.findColour (e.hasKeyboardFocus (true) ? juce::TextEditor::focusedOutlineColourId
                                                         : juce::TextEditor::outlineColourId));
    g.drawRect (juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f), hairline);
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&) { return mono (12.0f); }

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int,
                                juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRect (r);
    g.setColour (box.findColour (box.hasKeyboardFocus (true) ? juce::ComboBox::arrowColourId
                                                             : juce::ComboBox::outlineColourId));
    g.drawRect (r, hairline);

    // The one mark that is not type: which way the list opens.
    juce::Path caret;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    caret.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (caret);
}

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 26, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font LookAndFeel::getPopupMenuFont() { return mono (12.0f); }

void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (hue::paper);
    g.setColour (ink (0.32f));
    g.drawRect (0, 0, w, h);
}

juce::Font LookAndFeel::getAlertWindowMessageFont() { return mono (12.0f); }
juce::Font LookAndFeel::getAlertWindowTitleFont()   { return mono (12.0f, true); }

void LookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow& w,
                                const juce::Rectangle<int>& textArea, juce::TextLayout& text)
{
    g.fillAll (hue::paper);
    g.setColour (ink (0.32f));
    g.drawRect (w.getLocalBounds(), 1);
    text.draw (g, textArea.toFloat());
}

} // namespace skjar::sungam
