import React from 'react';
import './InputBox.css';


export default function InputBox({ value, onChange, onSubmit, disabled }) {

    function handleKeyPress(event) {
        if (event.key === "Enter" && event.shiftKey) {
            // Shift + Enter is pressed
            // Insert newline in the textbox
          } else if (event.key === "Enter") {
            // Only Enter is pressed
            onSubmit();
          }
    }


    return (
        <div className="InputWrapper">
            <input 
                className="InputBox" 
                spellCheck="false" 
                placeholder="What's on your mind?"
                value={value} 
                onChange={onChange} 
                onKeyDown={handleKeyPress}></input>
            <button disabled={disabled} className="Button" onClick={onSubmit}>Send</button>
        </div>
    )
}