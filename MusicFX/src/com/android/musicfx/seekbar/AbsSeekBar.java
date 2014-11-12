/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.musicfx.seekbar;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

public abstract class AbsSeekBar extends ProgressBar {
    private Drawable mThumb;
    private int mThumbOffset;
    
    /**
     * On touch, this offset plus the scaled value from the position of the
     * touch will form the progress value. Usually 0.
     */
    float mTouchProgressOffset;

    /**
     * Whether this is user seekable.
     */
    boolean mIsUserSeekable = true;

    boolean mIsVertical = false;
    /**
     * On key presses (right or left), the amount to increment/decrement the
     * progress.
     */
    private int mKeyProgressIncrement = 1;
    
    private static final int NO_ALPHA = 0xFF;
    private float mDisabledAlpha;
    
    private int mScaledTouchSlop;
    private float mTouchDownX;
    private float mTouchDownY;
    private boolean mIsDragging;

    public AbsSeekBar(Context context) {
        super(context);
    }

    public AbsSeekBar(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public AbsSeekBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        TypedArray a = context.obtainStyledAttributes(attrs,
                com.android.internal.R.styleable.SeekBar, defStyle, 0);
        Drawable thumb = a.getDrawable(com.android.internal.R.styleable.SeekBar_thumb);
        setThumb(thumb); // will guess mThumbOffset if thumb != null...
        // ...but allow layout to override this
        int thumbOffset = a.getDimensionPixelOffset(
                com.android.internal.R.styleable.SeekBar_thumbOffset, getThumbOffset());
        setThumbOffset(thumbOffset);
        a.recycle();

        a = context.obtainStyledAttributes(attrs,
                com.android.internal.R.styleable.Theme, 0, 0);
        mDisabledAlpha = a.getFloat(com.android.internal.R.styleable.Theme_disabledAlpha, 0.5f);
        a.recycle();

        mScaledTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
    }

    /**
     * Sets the thumb that will be drawn at the end of the progress meter within the SeekBar.
     * <p>
     * If the thumb is a valid drawable (i.e. not null), half its width will be
     * used as the new thumb offset (@see #setThumbOffset(int)).
     * 
     * @param thumb Drawable representing the thumb
     */
    public void setThumb(Drawable thumb) {
        boolean needUpdate;
        // This way, calling setThumb again with the same bitmap will result in
        // it recalcuating mThumbOffset (if for example it the bounds of the
        // drawable changed)
        if (mThumb != null && thumb != mThumb) {
            mThumb.setCallback(null);
            needUpdate = true;
        } else {
            needUpdate = false;
        }
        if (thumb != null) {
            thumb.setCallback(this);

            // Assuming the thumb drawable is symmetric, set the thumb offset
            // such that the thumb will hang halfway off either edge of the
            // progress bar.
            if (mIsVertical) {
                mThumbOffset = thumb.getIntrinsicHeight() / 2;
            } else {
                mThumbOffset = thumb.getIntrinsicWidth() / 2;
            }

            // If we're updating get the new states
            if (needUpdate &&
                    (thumb.getIntrinsicWidth() != mThumb.getIntrinsicWidth()
                        || thumb.getIntrinsicHeight() != mThumb.getIntrinsicHeight())) {
                requestLayout();
            }
        }
        mThumb = thumb;
        invalidate();
        if (needUpdate) {
            updateThumbPos(getWidth(), getHeight());
            if (thumb.isStateful()) {
                // Note that if the states are different this won't work.
                // For now, let's consider that an app bug.
                int[] state = getDrawableState();
                thumb.setState(state);
            }
        }
    }

    /**
     * @see #setThumbOffset(int)
     */
    public int getThumbOffset() {
        return mThumbOffset;
    }

    /**
     * Sets the thumb offset that allows the thumb to extend out of the range of
     * the track.
     * 
     * @param thumbOffset The offset amount in pixels.
     */
    public void setThumbOffset(int thumbOffset) {
        mThumbOffset = thumbOffset;
        invalidate();
    }

    /**
     * Sets the amount of progress changed via the arrow keys.
     * 
     * @param increment The amount to increment or decrement when the user
     *            presses the arrow keys.
     */
    public void setKeyProgressIncrement(int increment) {
        mKeyProgressIncrement = increment < 0 ? -increment : increment;
    }

    /**
     * Returns the amount of progress changed via the arrow keys.
     * <p>
     * By default, this will be a value that is derived from the max progress.
     * 
     * @return The amount to increment or decrement when the user presses the
     *         arrow keys. This will be positive.
     */
    public int getKeyProgressIncrement() {
        return mKeyProgressIncrement;
    }
    
    @Override
    public synchronized void setMax(int max) {
        super.setMax(max);

        if ((mKeyProgressIncrement == 0) || (getMax() / mKeyProgressIncrement > 20)) {
            // It will take the user too long to change this via keys, change it
            // to something more reasonable
            setKeyProgressIncrement(Math.max(1, Math.round((float) getMax() / 20)));
        }
    }

    @Override
    protected boolean verifyDrawable(Drawable who) {
        return who == mThumb || super.verifyDrawable(who);
    }

    @Override
    public void jumpDrawablesToCurrentState() {
        super.jumpDrawablesToCurrentState();
        if (mThumb != null) mThumb.jumpToCurrentState();
    }

    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        
        Drawable progressDrawable = getProgressDrawable();
        if (progressDrawable != null) {
            progressDrawable.setAlpha(isEnabled() ? NO_ALPHA : (int) (NO_ALPHA * mDisabledAlpha));
        }
        
        if (mThumb != null && mThumb.isStateful()) {
            int[] state = getDrawableState();
            mThumb.setState(state);
        }
    }
    
    @Override
    void onProgressRefresh(float scale, boolean fromUser) {
        super.onProgressRefresh(scale, fromUser);
        Drawable thumb = mThumb;
        if (thumb != null) {
            setThumbPos(getWidth(), getHeight(), thumb, scale, Integer.MIN_VALUE);
            /*
             * Since we draw translated, the drawable's bounds that it signals
             * for invalidation won't be the actual bounds we want invalidated,
             * so just invalidate this whole view.
             */
            invalidate();
        }
    }
    
    
    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        updateThumbPos(w, h);
    }

    private void updateThumbPos(int w, int h) {
        Drawable d = getCurrentDrawable();
        Drawable thumb = mThumb;
        if (mIsVertical) {
            int thumbWidth = thumb == null ? 0 : thumb.getIntrinsicWidth();
            // The max width does not incorporate padding, whereas the width
            // parameter does
            int trackWidth = Math.min(mMaxWidth, w - mPaddingLeft - mPaddingRight);

            int max = getMax();
            float scale = max > 0 ? (float) getProgress() / (float) max : 0;

            if (thumbWidth > trackWidth) {
                if (thumb != null) {
                    setThumbPos(w, h, thumb, scale, 0);
                }
                int gapForCenteringTrack = (thumbWidth - trackWidth) / 2;
                if (d != null) {
                    // Canvas will be translated by the padding, so 0,0 is where we start drawing
                    d.setBounds(gapForCenteringTrack, 0,
                            w - mPaddingRight - gapForCenteringTrack - mPaddingLeft,
                            h - mPaddingBottom - mPaddingTop);
                }
            } else {
                if (d != null) {
                    // Canvas will be translated by the padding, so 0,0 is where we start drawing
                    d.setBounds(0, 0, w - mPaddingRight - mPaddingLeft, h - mPaddingBottom
                            - mPaddingTop);
                }
                int gap = (trackWidth - thumbWidth) / 2;
                if (thumb != null) {
                    setThumbPos(w, h, thumb, scale, gap);
                }
            }
        } else {
            int thumbHeight = thumb == null ? 0 : thumb.getIntrinsicHeight();
            // The max height does not incorporate padding, whereas the height
            // parameter does
            int trackHeight = Math.min(mMaxHeight, h - mPaddingTop - mPaddingBottom);

            int max = getMax();
            float scale = max > 0 ? (float) getProgress() / (float) max : 0;

            if (thumbHeight > trackHeight) {
                if (thumb != null) {
                    setThumbPos(w, h, thumb, scale, 0);
                }
                int gapForCenteringTrack = (thumbHeight - trackHeight) / 2;
                if (d != null) {
                    // Canvas will be translated by the padding, so 0,0 is where we start drawing
                    d.setBounds(0, gapForCenteringTrack,
                            w - mPaddingRight - mPaddingLeft, h - mPaddingBottom - gapForCenteringTrack
                            - mPaddingTop);
                }
            } else {
                if (d != null) {
                    // Canvas will be translated by the padding, so 0,0 is where we start drawing
                    d.setBounds(0, 0, w - mPaddingRight - mPaddingLeft, h - mPaddingBottom
                            - mPaddingTop);
                }
                int gap = (trackHeight - thumbHeight) / 2;
                if (thumb != null) {
                    setThumbPos(w, h, thumb, scale, gap);
                }
            }
        }
    }

    /**
     * @param gap If set to {@link Integer#MIN_VALUE}, this will be ignored and
     */
    private void setThumbPos(int w, int h, Drawable thumb, float scale, int gap) {
        int available;
        int thumbWidth = thumb.getIntrinsicWidth();
        int thumbHeight = thumb.getIntrinsicHeight();
        if (mIsVertical) {
            available = h - mPaddingTop - mPaddingBottom - thumbHeight;
        } else {
            available = w - mPaddingLeft - mPaddingRight - thumbWidth;
        }

        // The extra space for the thumb to move on the track
        available += mThumbOffset * 2;


        if (mIsVertical) {
            int thumbPos = (int) ((1.0f - scale) * available);
            int leftBound, rightBound;
            if (gap == Integer.MIN_VALUE) {
                Rect oldBounds = thumb.getBounds();
                leftBound = oldBounds.left;
                rightBound = oldBounds.right;
            } else {
                leftBound = gap;
                rightBound = gap + thumbWidth;
            }

            // Canvas will be translated, so 0,0 is where we start drawing
            thumb.setBounds(leftBound, thumbPos, rightBound, thumbPos + thumbHeight);
        } else {
            int thumbPos = (int) (scale * available);
            int topBound, bottomBound;
            if (gap == Integer.MIN_VALUE) {
                Rect oldBounds = thumb.getBounds();
                topBound = oldBounds.top;
                bottomBound = oldBounds.bottom;
            } else {
                topBound = gap;
                bottomBound = gap + thumbHeight;
            }

            // Canvas will be translated, so 0,0 is where we start drawing
            thumb.setBounds(thumbPos, topBound, thumbPos + thumbWidth, bottomBound);
        }
    }
    
    @Override
    protected synchronized void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mThumb != null) {
            canvas.save();
            // Translate the padding. For the x/y, we need to allow the thumb to
            // draw in its extra space
            if (mIsVertical) {
                canvas.translate(mPaddingLeft, mPaddingTop - mThumbOffset);
                mThumb.draw(canvas);
                canvas.restore();
            } else {
                canvas.translate(mPaddingLeft - mThumbOffset, mPaddingTop);
                mThumb.draw(canvas);
                canvas.restore();
            }
        }
    }

    @Override
    protected synchronized void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        Drawable d = getCurrentDrawable();

        int thumbHeight = mThumb == null ? 0 : mThumb.getIntrinsicHeight();
        int dw = 0;
        int dh = 0;
        if (d != null) {
            dw = Math.max(mMinWidth, Math.min(mMaxWidth, d.getIntrinsicWidth()));
            dh = Math.max(mMinHeight, Math.min(mMaxHeight, d.getIntrinsicHeight()));
            dh = Math.max(thumbHeight, dh);
        }
        dw += mPaddingLeft + mPaddingRight;
        dh += mPaddingTop + mPaddingBottom;
        
        setMeasuredDimension(resolveSizeAndState(dw, widthMeasureSpec, 0),
                resolveSizeAndState(dh, heightMeasureSpec, 0));

        // TODO should probably make this an explicit attribute instead of implicitly
        // setting it based on the size
        if (getMeasuredHeight() > getMeasuredWidth()) {
            mIsVertical = true;
        }
    }
    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!mIsUserSeekable || !isEnabled()) {
            return false;
        }
        
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                if (isInScrollingContainer()) {
                    mTouchDownX = event.getX();
                    mTouchDownY = event.getY();
                } else {
                    setPressed(true);
                    if (mThumb != null) {
                        invalidate(mThumb.getBounds()); // This may be within the padding region
                    }
                    onStartTrackingTouch();
                    trackTouchEvent(event);
                    attemptClaimDrag();
                }
                break;
                
            case MotionEvent.ACTION_MOVE:
                if (mIsDragging) {
                    trackTouchEvent(event);
                } else {
                    final float x = event.getX();
                    final float y = event.getX();
                    if (Math.abs(mIsVertical ?
                            (y - mTouchDownY) : (x - mTouchDownX)) > mScaledTouchSlop) {
                        setPressed(true);
                        if (mThumb != null) {
                            invalidate(mThumb.getBounds()); // This may be within the padding region
                        }
                        onStartTrackingTouch();
                        trackTouchEvent(event);
                        attemptClaimDrag();
                    }
                }
                break;
                
            case MotionEvent.ACTION_UP:
                if (mIsDragging) {
                    trackTouchEvent(event);
                    onStopTrackingTouch();
                    setPressed(false);
                } else {
                    // Touch up when we never crossed the touch slop threshold should
                    // be interpreted as a tap-seek to that location.
                    onStartTrackingTouch();
                    trackTouchEvent(event);
                    onStopTrackingTouch();
                }
                // ProgressBar doesn't know to repaint the thumb drawable
                // in its inactive state when the touch stops (because the
                // value has not apparently changed)
                invalidate();
                break;
                
            case MotionEvent.ACTION_CANCEL:
                if (mIsDragging) {
                    onStopTrackingTouch();
                    setPressed(false);
                }
                invalidate(); // see above explanation
                break;
        }
        return true;
    }

    private void trackTouchEvent(MotionEvent event) {
        float progress = 0;
        if (mIsVertical) {
            final int height = getHeight();
            final int available = height - mPaddingTop - mPaddingBottom;
            int y = (int)event.getY();
            float scale;
            if (y < mPaddingTop) {
                scale = 1.0f;
            } else if (y > height - mPaddingBottom) {
                scale = 0.0f;
            } else {
                scale = 1.0f - (float)(y - mPaddingTop) / (float)available;
                progress = mTouchProgressOffset;
            }

            final int max = getMax();
            progress += scale * max;
        } else {
            final int width = getWidth();
            final int available = width - mPaddingLeft - mPaddingRight;
            int x = (int)event.getX();
            float scale;
            if (x < mPaddingLeft) {
                scale = 0.0f;
            } else if (x > width - mPaddingRight) {
                scale = 1.0f;
            } else {
                scale = (float)(x - mPaddingLeft) / (float)available;
                progress = mTouchProgressOffset;
            }

            final int max = getMax();
            progress += scale * max;
        }
        
        setProgress((int) progress, true);
    }

    /**
     * Tries to claim the user's drag motion, and requests disallowing any
     * ancestors from stealing events in the drag.
     */
    private void attemptClaimDrag() {
        if (mParent != null) {
            mParent.requestDisallowInterceptTouchEvent(true);
        }
    }
    
    /**
     * This is called when the user has started touching this widget.
     */
    void onStartTrackingTouch() {
        mIsDragging = true;
    }

    /**
     * This is called when the user either releases his touch or the touch is
     * canceled.
     */
    void onStopTrackingTouch() {
        mIsDragging = false;
    }

    /**
     * Called when the user changes the seekbar's progress by using a key event.
     */
    void onKeyChange() {
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (isEnabled()) {
            int progress = getProgress();
            if ((keyCode == KeyEvent.KEYCODE_DPAD_LEFT && !mIsVertical)
                    || (keyCode == KeyEvent.KEYCODE_DPAD_DOWN && mIsVertical)) {
                if (progress > 0) {
                    setProgress(progress - mKeyProgressIncrement, true);
                    onKeyChange();
                    return true;
                }
            } else if ((keyCode == KeyEvent.KEYCODE_DPAD_RIGHT && !mIsVertical)
                    || (keyCode == KeyEvent.KEYCODE_DPAD_UP && mIsVertical)) {
                if (progress < getMax()) {
                    setProgress(progress + mKeyProgressIncrement, true);
                    onKeyChange();
                    return true;
                }
            }
        }

        return super.onKeyDown(keyCode, event);
    }

}
