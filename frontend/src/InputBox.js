import React, { useState } from 'react';
import './InputBox.css';


export default function InputBox({value, onChange, onSubmit}) {
   return (
    <div className="InputWrapper">
        <input className="InputBox" spellCheck="false" placeholder='What is the question?' value={value} onChange={onChange}></input>
        <button className="Button" onClick={onSubmit}>Send</button>
    </div>
   )
}